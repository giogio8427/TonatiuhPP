#include "CmdLightKitModified.h"

#include <Inventor/nodekits/SoSceneKit.h>
#include <Inventor/nodekits/SoNodeKitListPart.h>
#include "libraries/geometry/gcf.h"
#include "tree/SceneModel.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/sun/SunKit.h"

/**
 * Creates a new lightKit modification command that represents a new light defined as \a newLightKit to the \a scene.
 *
 * If the model has not previous light a light node is added to \a sceneModel.
 */

CmdLightKitModified::CmdLightKitModified(
    SunKit* lightKitNew,
    SoSceneKit* sceneKit,
    SceneModel& sceneModel,
    QUndoCommand* parent
):
    QUndoCommand("Modify LightKit", parent),
    m_hasOld(false),
    m_lightKitNew(0),
    m_sunShapeOld(0),
    m_azimuthOld(0),
    m_zenithOld(0),
    m_nodesOld(""),
    m_sceneKit(sceneKit),
    m_sceneModel(&sceneModel)
{
    if (!lightKitNew)
        gcf::SevereError("CmdLightKitModified called with NULL SunKit*");

    m_lightKitNew = static_cast<SunKit*>(lightKitNew->copy(true));
    m_lightKitNew->ref();

    SunKit* lightKit = dynamic_cast<SunKit*>(m_sceneKit->getPart("lightList[0]", false));
    if (lightKit) {
        m_hasOld = true;
        m_sunShapeOld = dynamic_cast<SunShape*>(lightKit->getPart("tsunshape", false)->copy(true) );
        if (m_sunShapeOld) m_sunShapeOld->ref();
//        m_azimuthOld = lightKit->azimuth.getValue();
//        m_zenithOld = lightKit->zenith.getValue();
        m_nodesOld = lightKit->disabledNodes.getValue().getString();
    }
}

/*!
 * Destroys the CmdLightKitModified object.
 */
CmdLightKitModified::~CmdLightKitModified()
{
    m_lightKitNew->unref();
    m_sunShapeOld->unref();
}

/*!
 * Reverts to the previous light. After undo() is called, the state of the scene will be the same as before redo() was called.
 *  * \sa redo().
 */
void CmdLightKitModified::undo()
{
    if (m_hasOld) {
        SunKit* lightKit = static_cast<SunKit*> (m_sceneKit->getPart("lightList[0]", false) );
        lightKit->setPart("tsunshape", m_sunShapeOld);
        lightKit->updatePosition();
        lightKit->disabledNodes.setValue(m_nodesOld.toStdString().c_str() );
    } else
        m_sceneModel->RemoveLightNode(*m_lightKitNew);
}

/*!
 * Applies a change to the light. After redo() scene will contain the light with the new definition.
 * \sa undo().
 */
void CmdLightKitModified::redo()
{
    if (m_hasOld) {
        SunKit* lightKit = static_cast<SunKit*>(m_sceneKit->getPart("lightList[0]", false));
        SunShape* shape = static_cast<SunShape*>(m_lightKitNew->getPart("tsunshape", false));
        lightKit->setPart("tsunshape", shape);
        lightKit->updatePosition();
        lightKit->disabledNodes.setValue(m_lightKitNew->disabledNodes.getValue() );
    } else
        m_sceneModel->InsertLightNode(*m_lightKitNew);
}
