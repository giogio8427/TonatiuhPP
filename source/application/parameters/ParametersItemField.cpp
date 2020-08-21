#include "ParametersItemField.h"

#include <Inventor/fields/SoField.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include "ParametersItemNode.h"

void ParametersItemField::updateItem(void* data, SoSensor*)
{
    ParametersItemField* item = (ParametersItemField*) data;
    if (SoSFNode* f = dynamic_cast<SoSFNode*>(item->m_field))
    {
        QStandardItem* qitem = item->parent()->child(item->row(), 0);
        ParametersItemNode* itemNode = (ParametersItemNode*) qitem;
        itemNode->setNode(f->getValue());
    }
    item->update();
}


ParametersItemField::ParametersItemField(SoField* field):
    QStandardItem(),
    m_field(field)
{
     m_sensor = new SoFieldSensor(updateItem, this);
     m_sensor->attach(m_field);
}

ParametersItemField::~ParametersItemField()
{
    delete m_sensor;
}

QVariant ParametersItemField::data(int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (SoSFNode* f = dynamic_cast<SoSFNode*>(m_field))
        {
            return f->getValue()->getTypeId().getName().getString();
        }
        if (SoSFBool* f = dynamic_cast<SoSFBool*>(m_field))
            return f->getValue() ? "true" : "false";
        else {
            SbString v;
            m_field->get(v);
            QString txt = v.getString();

            // discard multilines
            if (txt.indexOf('\n') >= 0)
                return QString("...");

            // correct precision
            QStringList list = txt.split(" ");
            QString ans;
            for (int n = 0; n < list.size(); ++n) {
                if (n > 0) ans += " ";
                QString q = list[n];
                bool ok;
                double d = q.toDouble(&ok);
                if (ok)
                    ans += QString::number(d);
                else
                    return txt;
            }
            return ans;
        }
    }
    else if (role == Qt::EditRole)
    {
        SbString v;
        m_field->get(v);
        return v.getString();
    }

    return QStandardItem::data(role);
}

void ParametersItemField::setData(const QVariant& value, int role)
{
    QStandardItem::setData(value, role);
}

void ParametersItemField::update()
{
    emitDataChanged();
}
