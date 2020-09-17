#include "NodeObject.h"

#include <QScriptEngine>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>

#include "kernel/scene/TSeparatorKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/shape/ShapeRT.h"
#include "kernel/profiles/ProfileRT.h"
#include "kernel/material/MaterialRT.h"
#include "kernel/trackers/TrackerArmature.h"
#include "kernel/trackers/TrackerKit.h"

#include "main/MainWindow.h"
#include "main/PluginManager.h"


MainWindow* NodeObject::m_mainWindow = 0;


NodeObject::NodeObject(QObject* parent):
    QObject(parent)
{
    m_node = new TSeparatorKit;
}

NodeObject::NodeObject(SoNode* node):
    QObject(0),
    m_node(node)
{

}

QScriptValue NodeObject::getScene()
{
    NodeObject* ans = new NodeObject(m_mainWindow->getSceneKit());
    return engine()->newQObject(ans);
}

QScriptValue NodeObject::getRoot()
{
    NodeObject* ans = new NodeObject(m_mainWindow->getSceneKit()->getLayout());
    return engine()->newQObject(ans);
}

QScriptValue NodeObject::createNode(const QString& name)
{
    if (m_node->getTypeId() != TSeparatorKit::getClassTypeId())
        return QScriptValue();
    TSeparatorKit* kit = new TSeparatorKit;

    TSeparatorKit* parent = (TSeparatorKit*) m_node;
    SoGroup* group = (SoGroup*) parent->getPart("group", true);
    group->addChild(kit);

    NodeObject* ans = new NodeObject(kit);
    if (!name.isEmpty()) ans->setName(name);
    return engine()->newQObject(ans);
}

QScriptValue NodeObject::createShape()
{
    if (m_node->getTypeId() != TSeparatorKit::getClassTypeId()) return 0;
    TShapeKit* kit = new TShapeKit;

    TSeparatorKit* parent = (TSeparatorKit*) m_node;
    SoGroup* group = (SoGroup*) parent->getPart("group", true);
    group->addChild(kit);

    NodeObject* ans = new NodeObject(kit);
    return engine()->newQObject(ans);
}

QScriptValue NodeObject::createTracker()
{
    if (m_node->getTypeId() != TSeparatorKit::getClassTypeId()) return 0;
    TrackerKit* kit = new TrackerKit;

    TSeparatorKit* parent = (TSeparatorKit*) m_node;
    SoGroup* group = (SoGroup*) parent->getPart("group", true);
    group->addChild(kit);

    NodeObject* ans = new NodeObject(kit);
    return engine()->newQObject(ans);
}

QScriptValue NodeObject::getPart(const QString& name)
{
    if (!m_node->getTypeId().isDerivedFrom(SoBaseKit::getClassTypeId())) return 0;

    SoBaseKit* kit = (SoBaseKit*)(m_node);
    SoNode* node = kit->getPart(name.toLatin1().data(), true);
    if (!node) {
        SoField* field = m_node->getField(name.toLatin1().data());
        SoSFNode* fnode = dynamic_cast<SoSFNode*>(field);
        if (fnode) node = fnode->getValue();
    }
    if (!node) return 0;
    NodeObject* ans = new NodeObject(node);
    return engine()->newQObject(ans);
}

QScriptValue NodeObject::insertSurface(const QString& name)
{
    if (m_node->getTypeId() != TShapeKit::getClassTypeId()) return 0;

    ShapeFactory* f = m_mainWindow->getPlugins()->getShapeMap().value(name, 0);
    ShapeRT* shape = f->create();

    TShapeKit* parent = (TShapeKit*)(m_node);
    parent->shapeRT = shape;

    NodeObject* ans = new NodeObject(shape);
    return engine()->newQObject(ans);
}

QScriptValue NodeObject::insertProfile(const QString& name)
{
    if (m_node->getTypeId() != TShapeKit::getClassTypeId()) return 0;

    ProfileFactory* f = m_mainWindow->getPlugins()->getProfileMap().value(name, 0);
    ProfileRT* profile = f->create();

    TShapeKit* parent = (TShapeKit*)(m_node);
    parent->profileRT = profile;

    NodeObject* ans = new NodeObject(profile);
    return engine()->newQObject(ans);
}

QScriptValue NodeObject::insertMaterial(const QString& name)
{
    if (m_node->getTypeId() != TShapeKit::getClassTypeId()) return 0;

    MaterialFactory* f = m_mainWindow->getPlugins()->getMaterialMap().value(name, 0);
    MaterialRT* material = f->create();

    TShapeKit* parent = (TShapeKit*)(m_node);
    parent->materialRT = material;

    NodeObject* ans = new NodeObject(material);
    return engine()->newQObject(ans);
}

QScriptValue NodeObject::insertArmature(const QString& name)
{
    if (m_node->getTypeId() != TrackerKit::getClassTypeId()) return 0;

    TrackerFactory* f = m_mainWindow->getPlugins()->getTrackerMap().value(name, 0);
    TrackerArmature* tracker = f->create();

    TrackerKit* parent = (TrackerKit*)(m_node);
    parent->armature = tracker;

    NodeObject* ans = new NodeObject(tracker);
    return engine()->newQObject(ans);
}

void NodeObject::setName(const QString& name)
{
    m_node->setName(name.toLatin1().data());
}

void NodeObject::setParameter(const QString& name, const QString& value)
{
    SoNode* node = m_node;
    if (m_node->getTypeId() == TSeparatorKit::getClassTypeId()) {
        TSeparatorKit* parent = (TSeparatorKit*) m_node;
        node = parent->getPart("transform", true);
    }

    SoField* field = node->getField(name.toLatin1().data());
    if (field) field->set(value.toLatin1().data());
}
