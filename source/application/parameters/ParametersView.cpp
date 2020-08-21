#include "ParametersView.h"

#include "ParametersDelegate.h"
#include "ParametersModel.h"


ParametersView::ParametersView(QWidget* parent):
    QTreeView(parent)
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    ParametersDelegate* delegate = new ParametersDelegate(this);
    setItemDelegate(delegate);

    connect(
        this, SIGNAL(pressed(QModelIndex)),
        this, SLOT(onPressed(QModelIndex))
    );

    setStyleSheet(R"(
QLineEdit {

border: 1px solid #c8dbe5;

selection-background-color: #c8dbe5;
selection-color: black;
}
     )");
}

ParametersModel* ParametersView::getModel()
{
    return (ParametersModel*) model();
}

void ParametersView::reset()
{
    QTreeView::reset(); // keep one model to avoid reset?
    double w = 3*fontMetrics().horizontalAdvance("Parameter");
    setColumnWidth(0, w);
//    expandToDepth(1);

    if (ParametersModel* m = getModel()) {
        QStandardItem* root = m->invisibleRootItem();
        root = root->child(0);
        if (root) setRootIndex(root->index());
    }
    expandAll();
}

void ParametersView::onPressed(const QModelIndex& index)
{
    if (model()->flags(index) & Qt::ItemIsEditable) {
//        setSelectionBehavior(QAbstractItemView::SelectItems);
//        selectionModel()->select(index, QItemSelectionModel::Clear);
        edit(index);
//        selectionModel()->select(index, QItemSelectionModel::Select);
//        setSelectionBehavior(QAbstractItemView::SelectRows);
    }
}

