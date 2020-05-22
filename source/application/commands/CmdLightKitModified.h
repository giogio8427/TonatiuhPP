#pragma once

#include <QUndoCommand>

class SceneModel;
class SoSceneKit;
class SunKit;
class ShapeRT;
class SunShape;

//!  CmdLightKitModified class is the light definition command stored in the command stack.
/*!
  CmdLightKitModified represents a modification of the scene light. The modification could be a new light definition,
  a modificaiton of the light position on the scene or change on the sun shape parameters value.
   \sa CmdLightPositionModified
*/

class CmdLightKitModified: public QUndoCommand
{
public:
    CmdLightKitModified(
        SunKit* lightKit,
        SoSceneKit* sceneKit,
        SceneModel& sceneModel,
        QUndoCommand* parent = 0
     );
    ~CmdLightKitModified();

     void undo();
     void redo();

private:
    bool m_hasOld;
    SunKit* m_lightKitNew;
    SunShape* m_sunShapeOld;
    double m_azimuthOld;
    double m_zenithOld;
    QString m_nodesOld;

    SoSceneKit* m_sceneKit;
    SceneModel* m_sceneModel;
};
