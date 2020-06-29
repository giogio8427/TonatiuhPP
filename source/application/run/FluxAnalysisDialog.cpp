#include "FluxAnalysisDialog.h"
#include "ui_FluxAnalysisDialog.h"

#include <cmath>

#include <QFileDialog>
#include <QFutureWatcher>
#include <QIntValidator>
#include <QMessageBox>
#include <QMutex>
#include <QPair>
#include <QProgressDialog>
#include <QtConcurrentMap>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoTransform.h>

#include "FluxAnalysis.h"
#include "SelectSurfaceDialog.h"
#include "tree/SceneModel.h"
#include "kernel/air/Air.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/random/Random.h"
#include "kernel/run/RayTracer.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/trf.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/sun/SunKit.h"
#include "kernel/sun/SunAperture.h"
#include "libraries/geometry/Transform.h"
#include "libraries/geometry/gcf.h"



FluxAnalysisDialog::FluxAnalysisDialog(TSceneKit* sceneKit, SceneModel& sceneModel,
                                       InstanceNode* rootInstance,
                                       int sunWidthDivisions, int sunHeightDivisions,
                                       Random* randomDeviate,  QWidget* parent):
    QDialog(parent),
    ui(new Ui::FluxAnalysisDialog),
    m_sceneModel(&sceneModel),
    m_fluxLabelString("Flux, W/m^2")
{
    ui->setupUi(this);

    m_fluxAnalysis = new FluxAnalysis(sceneKit, sceneModel, rootInstance, sunWidthDivisions, sunHeightDivisions, randomDeviate);

    connect(ui->selectButton, SIGNAL(clicked()), this, SLOT(SelectSurface()));
    connect(ui->sidesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ChangeCurrentSurfaceSide()) );
    connect(ui->surfaceEdit, SIGNAL(editingFinished()), this, SLOT(ChangeCurrentSurface()) );
    connect(ui->runButton, SIGNAL(clicked()), this, SLOT(Run()) );
    connect(ui->hSectorXCoordSpin, SIGNAL(valueChanged(double)), this, SLOT(UpdateSectorPlotSlot()) );
    connect(ui->hSectorYCoordSpin, SIGNAL(valueChanged(double)), this, SLOT(UpdateSectorPlotSlot()) );
    connect(ui->spinCellsX, SIGNAL(editingFinished()), this, SLOT(UpdateAnalysis()) );
    connect(ui->spinCellsY, SIGNAL(editingFinished()), this, SLOT(UpdateAnalysis()) );
    connect(ui->lengthUnitLine, SIGNAL(editingFinished()), this, SLOT(UpdateLabelsUnits()) );
    connect(ui->powerUnitLine, SIGNAL(editingFinished()), this, SLOT(UpdateLabelsUnits()) );
    connect(ui->selectFileButton, SIGNAL(clicked()), this, SLOT(SelectExportFile()) );
    connect(ui->exportButton, SIGNAL(clicked()), this, SLOT(ExportData()) );
    connect(ui->storeTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(SaveCoordsExport()) );

    ui->plotFxy->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    ui->plotFxy->axisRect()->setupFullAxesBox(true);
    ui->plotFxy->plotLayout()->insertRow(0);
    ui->plotFxy->plotLayout()->addElement(0, 0, new QCPTextElement(ui->plotFxy, "Flux Distribution") );
    ui->plotFxy->xAxis->setLabel("x, m");
    ui->plotFxy->yAxis->setLabel("y, m");

    ui->plotFx->xAxis->setLabel("x, m");
    ui->plotFx->yAxis->setLabel("Flux, W/m^2");
    ui->plotFx->yAxis->setRange(0, 1.08);

    ui->plotFy->xAxis->setLabel("y, m");
    ui->plotFy->yAxis->setLabel("Flux, W/m^2");
    ui->plotFy->yAxis->setRange(0, 1.08);

    int q = fontMetrics().height();
    resize(64*q, 48*q);
}

FluxAnalysisDialog::~FluxAnalysisDialog()
{
    delete m_fluxAnalysis;
    delete ui;
}

/*!
 * Opens a dialog to select a surface from the system.
 * If the selected surface is ok
 */
void FluxAnalysisDialog::SelectSurface()
{
    SelectSurfaceDialog dialog(*m_sceneModel, false, this);
    dialog.SetShapeTypeFilters({"Planar", "Parabolic", "Cylinder"});
    if (!dialog.exec()) return;

    QString selectedSurfaceURL = dialog.GetSelectedSurfaceURL();

    if (!selectedSurfaceURL.isEmpty() && selectedSurfaceURL != m_currentSurfaceURL)
    {
        m_fluxAnalysis->clearPhotonMap();
        ClearCurrentAnalysis();
        ui->surfaceEdit->setText(selectedSurfaceURL);
        m_currentSurfaceURL = selectedSurfaceURL;
        UpdateSurfaceSides(m_currentSurfaceURL);
    }
}

/*!
 * Clear current results and simulation.
 */
void FluxAnalysisDialog::ClearCurrentAnalysis()
{
    ui->plotFy->clearPlottables();
    ui->plotFy->yAxis->setRange(0, 1.05);
    ui->plotFy->replot();

    ui->plotFx->clearPlottables();
    ui->plotFx->yAxis->setRange(0, 1.05);
    ui->plotFx->replot();

    ui->plotFxy->clearPlottables();

    //see how many elements there are
    int elementCount = ui->plotFxy->plotLayout()->elementCount();
    //loop over the elements
    for (int i = 0; i < elementCount; i++)
    {
    //test to see if any of the layout elements are of QCPColorScale type
    if (qobject_cast<QCPColorScale*> (ui->plotFxy->plotLayout()->elementAt(i) ) )
    ui->plotFxy->plotLayout()->removeAt(i);
    //collapse the empty elements
    ui->plotFxy->plotLayout()->simplify();
    }
    ui->plotFxy->replot();

    ui->hSectorXCoordSpin->setValue(0.00);
    ui->hSectorXCoordSpin->setMinimum(0.00);
    ui->hSectorXCoordSpin->setMaximum(0.00);
    ui->hSectorYCoordSpin->setValue(0.00);
    ui->hSectorYCoordSpin->setMinimum(0.00);
    ui->hSectorYCoordSpin->setMaximum(0.00);

    ui->totalPowerValue->setText(QString::number(0.0));
    ui->minimumFluxValue->setText(QString::number(0.0));
    ui->averageFluxValue->setText(QString::number(0.0));
    ui->maximumFluxValue->setText(QString::number(0.0));

    ui->maxCoordinatesValue->setText(" ; ");
    ui->errorValue->setText(QString::number(0.0) );
    ui->uniformityValue->setText(QString::number(0.0) );
    ui->centroidValue->setText(" ; ");
}

/*!
 * Updates the sidesCombo to select the surface side
 */
void FluxAnalysisDialog::UpdateSurfaceSides(QString selectedSurfaceURL)
{
    ui->sidesCombo->clear();

    QString surfaceType = m_fluxAnalysis->GetSurfaceType(selectedSurfaceURL);

    if (surfaceType.isEmpty() ) return;

    if (surfaceType == "Planar" || surfaceType == "ShapeFlatDisk" || surfaceType == "Cylinder")
    {
        ui->sidesCombo->addItem("front");
        ui->sidesCombo->addItem("back");
    }
}

/*!
 * Changes current surface to the surface node defined in surface line.
 */
void FluxAnalysisDialog::ChangeCurrentSurface()
{
    QString selectedSurfaceURL = ui->surfaceEdit->text();
    if (!selectedSurfaceURL.isEmpty() && (selectedSurfaceURL != m_currentSurfaceURL) )
	{
        QString surfaceType = m_fluxAnalysis->GetSurfaceType(selectedSurfaceURL);
        if (!surfaceType.isEmpty() && (surfaceType == "Cylinder" || surfaceType == "ShapeFlatDisk" || surfaceType == "Planar") )
		{
			m_fluxAnalysis->clearPhotonMap();
            ui->appendCheck->setChecked(false);
            ui->appendCheck->setEnabled(false);
			ClearCurrentAnalysis();
            ui->surfaceEdit->setText(selectedSurfaceURL);
			m_currentSurfaceURL = selectedSurfaceURL;
            UpdateSurfaceSides(m_currentSurfaceURL);
		}
        else QMessageBox::warning(this, tr("Tonatiuh"),
                              tr("The surface url defined is not valid.") );
	}
    else ui->surfaceEdit->setText(m_currentSurfaceURL);
}

/*
 * Clear the current analysis if the surface side is modified
 */
void FluxAnalysisDialog::ChangeCurrentSurfaceSide()
{
    m_fluxAnalysis->clearPhotonMap();
    ui->appendCheck->setChecked(false);
    ui->appendCheck->setEnabled(false);
    ClearCurrentAnalysis();
}

/*
 * Set the coords check box according to the store type
 */
void FluxAnalysisDialog::SaveCoordsExport()
{
    if (ui->storeTypeCombo->currentText() == "IMAGE.PNG" || ui->storeTypeCombo->currentText() == "IMAGE.JPG")
    {
        ui->saveCoordsCheckBox->setChecked(false);
        ui->saveCoordsCheckBox->setDisabled(true);
    }
    else ui->saveCoordsCheckBox->setEnabled(true);
}

/*
 * Exports the flux distribution
 */
void FluxAnalysisDialog::ExportData()
{
	int** photonCounts = m_fluxAnalysis->photonCountsValue();
    if (!photonCounts || photonCounts == 0)
	{
        QString message = QString(tr("Nothing available to export, first run the simulation") );
        QMessageBox::warning(this,  "Tonatiuh", message);
		return;
	}

    if (ui->fileDirEdit->text().isEmpty() )
	{
        QString message = QString(tr("Directory not valid") );
        QMessageBox::warning(this,  "Tonatiuh", message);
		return;
	}

    if (ui->fileNameEdit->text().isEmpty() )
	{
        QString message = QString(tr("File name not valid") );
        QMessageBox::warning(this,  "Tonatiuh", message);
		return;
	}

    QString exportDirectory(ui->fileDirEdit->text() );
    QString storeType = ui->storeTypeCombo->currentText();
    QString exportFileName = ui->fileNameEdit->text();

    QFileInfo exportFileInfo(exportFileName);
    if (storeType == "ASCII")
	{
        if (ui->saveCoordsCheckBox->isChecked() )
            m_fluxAnalysis->ExportAnalysis(exportDirectory, exportFileName, true);
		else
            m_fluxAnalysis->ExportAnalysis(exportDirectory, exportFileName, false);

	}
    else if (storeType == "IMAGE.JPG")
	{
        if (exportFileInfo.completeSuffix().compare("jpg") ) exportFileName.append(".jpg");

        QFile exportFile(exportDirectory + "/" + exportFileName);
        exportFile.open(QIODevice::WriteOnly);
        ui->plotFxy->saveJpg(exportFile.fileName(), 0, 0, 1.0, -1);   //(QString &  fileName, int  width = 0, int  height = 0, double  scale = 1.0, int  quality = -1  )
		exportFile.close();
	}
    else if (storeType == "IMAGE.PNG")
	{
        if (exportFileInfo.completeSuffix().compare("png") ) exportFileName.append(".png");
        QFile exportFile(exportDirectory + "/" + exportFileName);
        exportFile.open(QIODevice::WriteOnly);
        ui->plotFxy->savePng(exportFile.fileName(), 0, 0, 1.0, -1);   //(QString &  fileName, int  width = 0, int  height = 0, double  scale = 1.0, int  quality = -1  )
		exportFile.close();
	}

    QString message = QString(tr("Export done successfully") );
    QMessageBox::information(this,  "Tonatiuh", message);
}

/*!
 * Runs flux analysis for current defined surface.
 */
void FluxAnalysisDialog::Run()
{
	QDateTime startTime = QDateTime::currentDateTime();

	//Check the surface
    if (m_currentSurfaceURL.isEmpty() )
	{
        QMessageBox::warning(this, "Tonatiuh",
                             tr("Select a valid surface.") );
		return;
	}

	//Check the number of rays
//    QString nOfRays = ui->spinRays->value();
//	int pos = 0;
//    if (m_pNOfRays->validate(nOfRays, pos) != QValidator::Acceptable)
//	{
//        QMessageBox::warning(this, "Tonatiuh",
//                             tr("The number of rays to trace must be a positive value.") );
//		return;
//	}

	//Check the grid division
//    QString widthDivisions = ui->spinCellsX->text();
//    QValidator::State widthState = m_pGridWidthVal->validate(widthDivisions, pos);

//    QString heightDivisions = ui->spinCellsY->text();
//    QValidator::State heightSate = m_pGridHeightVal->validate(heightDivisions, pos);

//    if ( (widthState != QValidator::Acceptable) || (heightSate != QValidator::Acceptable) )
//	{
//        QMessageBox::warning(this, "Tonatiuh",
//                             tr("The gird divisions size must be greater than 2.") );
//		return;
//	}

    QString surfaceSide = ui->sidesCombo->currentText();
    bool increasePhotonMap = (ui->appendCheck->isEnabled() && ui->appendCheck->isChecked() );
    m_fluxAnalysis->RunFluxAnalysis(m_currentSurfaceURL, surfaceSide, ui->spinRays->value(), increasePhotonMap, ui->spinCellsY->value(), ui->spinCellsX->value());

	UpdateAnalysis();
    ui->appendCheck->setEnabled(true);

	QDateTime endTime = QDateTime::currentDateTime();
    std::cout << "Elapsed time: " << startTime.secsTo(endTime) << std::endl;
}

/*
 * Calculate flux distribution and statistics
 */
void FluxAnalysisDialog::UpdateAnalysis()
{
//	int pos = 0;
//    QString withValue = ui->spinCellsX->text();
//    QValidator::State widthState = m_pGridWidthVal->validate(withValue, pos);

//    QString heightValue = ui->spinCellsY->text();
//    QValidator::State heightSate = m_pGridHeightVal->validate(heightValue, pos);

//    if ( (widthState != QValidator::Acceptable) || (heightSate != QValidator::Acceptable) )
//	{
//        QMessageBox::warning(this, "Tonatiuh",
//                             tr("The gird divisions size must be greater than 2.") );
//		return;
//	}

    int widthDivisions = ui->spinCellsX->value();
    int heightDivisions = ui->spinCellsY->value();

    m_fluxAnalysis->UpdatePhotonCounts(heightDivisions, widthDivisions);

	int** photonCounts = m_fluxAnalysis->photonCountsValue();
    if (!photonCounts || photonCounts == 0) return;

	ClearCurrentAnalysis();

	double xmin = m_fluxAnalysis->xminValue();
	double ymin = m_fluxAnalysis->yminValue();
	double xmax = m_fluxAnalysis->xmaxValue();
	double ymax = m_fluxAnalysis->ymaxValue();
	double wPhoton = m_fluxAnalysis->wPhotonValue();
	double totalPower = m_fluxAnalysis->totalPowerValue();

    double widthCell = (xmax - xmin) / widthDivisions;
    double heightCell = (ymax - ymin) / heightDivisions;
	double areaCell = widthCell * heightCell;
    double maximumFlux = (m_fluxAnalysis->maximumPhotonsValue() * wPhoton) / areaCell;
	double minimumFlux = 0;
	double totalFlux = totalPower / areaCell;
    double averageFlux = totalFlux / (widthDivisions * heightDivisions);
    double maxXCoord = xmin + (m_fluxAnalysis->maximumPhotonsXCoordValue() + 0.5) * widthCell;
    double maxYCoord = ymin + (m_fluxAnalysis->maximumPhotonsYCoordValue() + 0.5) * heightCell;
    double maximumFluxError = (m_fluxAnalysis->maximumPhotonsErrorValue() * wPhoton) / ( ( (xmax - xmin) / (widthDivisions - 1) ) * ( (ymax - ymin) / (heightDivisions - 1) ) );
    double error = fabs(maximumFlux - maximumFluxError) / maximumFlux;
    double gravityX = 0.;
    double gravityY = 0.;
	double E = 0;

    for (int xIndex = 0; xIndex < widthDivisions; ++xIndex)
	{
        for (int yIndex = 0; yIndex < heightDivisions; ++yIndex)
		{
			double cellFlux = photonCounts[yIndex][xIndex] * wPhoton / areaCell;
            if (minimumFlux > cellFlux) minimumFlux = cellFlux;

            gravityX += cellFlux*(xmin + (xIndex + 0.5)*widthCell);
            gravityY += cellFlux*(ymin + (yIndex + 0.5)*heightCell);
            E += (cellFlux - averageFlux)*(cellFlux - averageFlux);
		}
	}

    double standarDeviation = sqrt(E/(widthDivisions*heightDivisions));
    double uniformity = standarDeviation/averageFlux;
	gravityX /= totalFlux;
	gravityY /= totalFlux;

    UpdateStatistics(totalPower, minimumFlux, averageFlux, maximumFlux, maxXCoord, maxYCoord, error, uniformity, gravityX, gravityY);
    UpdateFluxMapPlot(photonCounts, wPhoton, widthDivisions, heightDivisions, xmin, ymin, xmax, ymax);
    CreateSectorPlots(xmin, ymin, xmax, ymax);
    UpdateSectorPlots(photonCounts, wPhoton, widthDivisions, heightDivisions, xmin, ymin, xmax, ymax, maximumFlux);
}

/*
 * Updates the statistics
 */
void FluxAnalysisDialog::UpdateStatistics(double totalPower, double minimumFlux, double averageFlux, double maximumFlux,
                                          double maxXCoord, double maxYCoord, double error, double uniformity, double gravityX, double gravityY)
{
    ui->totalPowerValue->setText(QString::number(totalPower) );
    ui->minimumFluxValue->setText(QString::number(minimumFlux) );
    ui->averageFluxValue->setText(QString::number(averageFlux) );
    ui->maximumFluxValue->setText(QString::number(maximumFlux) );
    ui->maxCoordinatesValue->setText(QString("%1 ; %2").arg(QString::number(maxXCoord), QString::number(maxYCoord) ) );
    ui->errorValue->setText(QString::number(error) );
    ui->uniformityValue->setText(QString::number(uniformity) );
    ui->centroidValue->setText(QString("%1 ; %2").arg(QString::number(gravityX), QString::number(gravityY) ) );
}

/*
 * Updates the flux map plot
 */
void FluxAnalysisDialog::UpdateFluxMapPlot(int** photonCounts, double wPhoton, int widthDivisions, int heightDivisions, double xmin, double ymin, double xmax, double ymax)
{
	//Delete previous colormap, scale
    ui->plotFxy->clearPlottables();
    ui->plotFxy->clearItems();

	//see how many elements there are
    int elementCount = ui->plotFxy->plotLayout()->elementCount();
	//loop over the elements
    for (int i = 0; i < elementCount; i++)
	{
        //test to see if any of the layout elements are of QCPColorScale type
        if (qobject_cast<QCPColorScale*>(ui->plotFxy->plotLayout()->elementAt(i) ) )
            ui->plotFxy->plotLayout()->removeAt(i);
        //collapse the empty elements
        ui->plotFxy->plotLayout()->simplify();
	}

	// Create a QCPColorMap object to draw flux distribution
    QCPColorMap* colorMap = new QCPColorMap(ui->plotFxy->xAxis, ui->plotFxy->yAxis);
//    ui->plotFxy->addPlottable(colorMap);

    colorMap->data()->setSize(widthDivisions, heightDivisions);   // we want the color map to have widthDivisions * heightDivisions data points
    colorMap->data()->setRange(QCPRange(xmin, xmax), QCPRange(ymin, ymax) );      // and span the coordinate range -4..4 in both key (x) and value (y) dimensions

	//Assign flux data
    double widthCell = (xmax - xmin)/widthDivisions;
    double heightCell = (ymax - ymin)/heightDivisions;
	double areaCell = widthCell * heightCell;
    for (int xIndex = 0; xIndex < widthDivisions; ++xIndex)
	{
        for (int yIndex = 0; yIndex < heightDivisions; ++yIndex)
		{
			double cellFlux = photonCounts[yIndex][xIndex] * wPhoton / areaCell;
            colorMap->data()->setCell(xIndex, yIndex, cellFlux);
		}
	}

	// add a color scale:
    QCPColorScale* colorScale = new QCPColorScale(ui->plotFxy);
    ui->plotFxy->plotLayout()->addElement(1, 1, colorScale);   // add it to the right of the main axis rect

    colorScale->setType(QCPAxis::atRight);   // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    colorMap->setColorScale(colorScale);   // associate the color map with the color scale
    colorScale->axis()->setLabel(m_fluxLabelString);

    // set the contour plot color
    colorMap->setGradient(QCPColorGradient::gpThermal);

	// rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
	colorMap->rescaleDataRange();

	// forbid to drag and zoom the colorscale
    colorScale->setRangeZoom(false);
    colorScale->setRangeDrag(false);

	// make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
    QCPMarginGroup* marginGroup = new QCPMarginGroup(ui->plotFxy);
    ui->plotFxy->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

	// rescale axes and update plot view
    ui->plotFxy->rescaleAxes();
    ui->plotFxy->replot();
}

/*
 * Create sector plots
 */
void FluxAnalysisDialog::CreateSectorPlots(double xmin, double ymin, double xmax, double ymax)
{
	// Create a vertical and horizontal line for sectors
    QCPItemLine* tickVLine  = new  QCPItemLine(ui->plotFxy);
    ui->hSectorXCoordSpin->setMinimum(xmin);
    ui->hSectorXCoordSpin->setMaximum(xmax);
    ui->hSectorXCoordSpin->setSingleStep((xmax - xmin)/10);
//    ui->plotFxy->addItem(tickVLine);

    tickVLine->start->setCoords(0, ymin - 1);
    tickVLine->end->setCoords(0, ymax + 1);
    tickVLine->setPen(QPen(QColor(137, 140, 140), 1) );

    QCPItemLine* tickHLine = new QCPItemLine(ui->plotFxy);
    ui->hSectorYCoordSpin->setMinimum(ymin);
    ui->hSectorYCoordSpin->setMaximum(ymax);
    ui->hSectorYCoordSpin->setSingleStep( (ymax - ymin) / 10);
//    ui->plotFxy->addItem(tickHLine);

    tickHLine->start->setCoords(xmin - 1,  0);
    tickHLine->end->setCoords(xmax + 1, 0);
    tickHLine->setPen(QPen(QColor(137, 140, 140), 1));
}

/*
 * Updates the sector plots
 */
void FluxAnalysisDialog::UpdateSectorPlots(int** photonCounts, double wPhoton, int widthDivisions, int heightDivisions, double xmin, double ymin, double xmax, double ymax, double maximumFlux)
{
    QCPItemLine* tickVLine = (QCPItemLine*) ui->plotFxy->item(0);
	QPointF pointVStart = tickVLine->start->coords();
    tickVLine->start->setCoords(ui->hSectorXCoordSpin->value(), pointVStart.y() );
    QPointF pointVEnd = tickVLine->end->coords();
    tickVLine->end->setCoords(ui->hSectorXCoordSpin->value(), pointVEnd.y() );
    tickVLine->setPen(QPen(QColor(137, 140, 140), 1) );

    QCPItemLine* tickHLine  = ( QCPItemLine* ) ui->plotFxy->item(1);
	QPointF pointHStart = tickHLine->start->coords();
    tickHLine->start->setCoords(pointHStart.x(), ui->hSectorYCoordSpin->value() );
	QPointF pointHEnd = tickHLine->end->coords();
    tickHLine->end->setCoords(pointHEnd.x(), ui->hSectorYCoordSpin->value() );
    tickHLine->setPen(QPen(QColor(137, 140, 140), 1) );

    ui->plotFxy->replot();

	//Delete previous plots
    ui->plotFy->clearPlottables();
    ui->plotFx->clearPlottables();

    double xCoordSector = ui->hSectorXCoordSpin->value();
    double yCoordSector = ui->hSectorYCoordSpin->value();

    double widthCell = (xmax - xmin) / widthDivisions;
    double heightCell = (ymax - ymin) / heightDivisions;
	double areaCell = widthCell * heightCell;

    int xbin1Index = floor( (xCoordSector - xmin) / (xmax - xmin) * widthDivisions);
    if (xbin1Index >= widthDivisions) xbin1Index = widthDivisions - 1;
    int ybin1Index = floor( (yCoordSector - ymin) / (ymax - ymin) * heightDivisions);
    if (ybin1Index >= heightDivisions) ybin1Index = heightDivisions - 1;

	// generate some data:
    QVector<double> verticalXValues(heightDivisions), verticalYValues(heightDivisions);     // initialize with entries 0..100
    for (int i = 0; i < heightDivisions; ++i)
	{
        verticalXValues[i] = ymin + (i + 0.5) * heightCell;
		verticalYValues[i] = photonCounts[i][xbin1Index] * wPhoton / areaCell;
	}

    QVector<double> horizontalXValues(widthDivisions), horizontalYValues(widthDivisions);     // initialize with entries 0..100
    for (int i = 0; i < widthDivisions; ++i)
	{
        horizontalXValues[i] = xmin + (i + 0.5) * widthCell;
		horizontalYValues[i] = photonCounts[ybin1Index][i] * wPhoton / areaCell;
	}

	// create graph and assign data to it:
    ui->plotFy->addGraph();
    ui->plotFy->graph(0)->setData(verticalXValues, verticalYValues);
	// set axes ranges, so we see all data:
    ui->plotFy->xAxis->setRange(ymin, ymax);
    ui->plotFy->yAxis->setRange(0, 1.2 * maximumFlux);
    //plotFy->rescaleAxes();
    ui->plotFy->replot();

	// create graph and assign data to it:
    ui->plotFx->addGraph();
    ui->plotFx->graph(0)->setData(horizontalXValues, horizontalYValues);
	// set axes ranges, so we see all data:
    ui->plotFx->xAxis->setRange(xmin, xmax);
    ui->plotFx->yAxis->setRange(0, 1.2*maximumFlux);
    ui->plotFx->replot();
}

/*
 * Updates the labels of the plots
 */
void FluxAnalysisDialog::UpdateLabelsUnits()
{
    QString lengthUnitString = ui->lengthUnitLine->text();
    if (lengthUnitString.isEmpty() ) lengthUnitString = "m";

    QString powerUnitString = ui->powerUnitLine->text();
    if (powerUnitString.isEmpty()) powerUnitString = "W";

    ui->plotFxy->xAxis->setLabel(QString("x, %1").arg(lengthUnitString) );
    ui->plotFxy->yAxis->setLabel(QString("y, %1").arg(lengthUnitString) );

    ui->plotFx->xAxis->setLabel(QString("y, %1").arg(lengthUnitString) );
    ui->plotFy->xAxis->setLabel(QString("x, %1").arg(lengthUnitString) );

    ui->plotFx->yAxis->setLabel(QString("Flux, %1/%2^2").arg(powerUnitString, lengthUnitString) );
    ui->plotFy->yAxis->setLabel(QString("Flux, %1/%2^2").arg(powerUnitString, lengthUnitString) );

    m_fluxLabelString = QString("Flux, %1/%2^2").arg(powerUnitString, lengthUnitString);

    QCPColorMap* colorMapPlot = qobject_cast<QCPColorMap*>(ui->plotFxy->plottable() );

	//see how many elements there are
    int elementCount = ui->plotFxy->plotLayout()->elementCount();
	//loop over the elements
    for (int i = 0; i < elementCount; i++) {
        //test to see if any of the layout elements are of QCPColorScale type
        if (qobject_cast<QCPColorScale*>(ui->plotFxy->plotLayout()->elementAt(i) ) )
            colorMapPlot->colorScale()->axis()->setLabel(m_fluxLabelString);
    }
    ui->plotFxy->replot();
    ui->plotFx->replot();
    ui->plotFy->replot();
}

/*
 * Update plots when changing x or y coord
 */
void FluxAnalysisDialog::UpdateSectorPlotSlot()
{
    int** photonCounts = m_fluxAnalysis->photonCountsValue();
    if (!photonCounts || photonCounts == 0) return;

    double xmin = m_fluxAnalysis->xminValue();
    double ymin = m_fluxAnalysis->yminValue();
    double xmax = m_fluxAnalysis->xmaxValue();
    double ymax = m_fluxAnalysis->ymaxValue();
    double wPhoton = m_fluxAnalysis->wPhotonValue();
    QString withValue = ui->spinCellsX->text();
    QString heightValue = ui->spinCellsY->text();
    int widthDivisions = withValue.toInt();
    int heightDivisions = heightValue.toInt();
    double widthCell = (xmax - xmin) / widthDivisions;
    double heightCell = (ymax - ymin) / heightDivisions;
    double areaCell = widthCell * heightCell;
    double maximumFlux = (m_fluxAnalysis->maximumPhotonsValue() * wPhoton) / areaCell;
    UpdateSectorPlots(photonCounts, wPhoton, widthDivisions, heightDivisions, xmin, ymin, xmax, ymax, maximumFlux);
}

/*
 * Select directory
 */
void FluxAnalysisDialog::SelectExportFile()
{
    QString path = QFileDialog::getExistingDirectory(this, "Choose a directory to save");
    ui->fileDirEdit->setText(path);
}



