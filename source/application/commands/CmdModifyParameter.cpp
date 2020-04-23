//!  CmdModifyParameter class is the parameter modification command stored in the command stack.
/*!
  CmdModifyParameter represents a modification of the scene element parameter. The modification could be for any element of the scene concentrator parameters.
  This class saves the new value of the parameter and old one to restore the previous state.
   \sa CmdLightPositionModified
*/

#include <Inventor/SbName.h>
#include <Inventor/SbString.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/nodes/SoNode.h>

#include "libraries/geometry/gf.h"

#include "CmdModifyParameter.h"
#include "kernel/gui/InstanceNode.h"
#include "gui/SceneModel.h"

/**
 * CmdModifyParameter creates a new command that represents a scene node parameter value modification. This object saves the \a parameterName parameter value of \a nodeIndex node and the new one
 * defined as \a parameterValue to apply and restore them.
 *
 * The \a parent is command object parent.
 */
CmdModifyParameter::CmdModifyParameter(  SoNode* node, QString parameterName, QString parameterValue, SceneModel* model, QUndoCommand* parent )
:QUndoCommand( parent ),
 m_coinNode(  node ),
 m_newValue ( parameterValue ),
 m_oldValue( "" ),
 m_parameterName( parameterName ),
 m_pModel( model )
{
    if( !m_coinNode )        gf::SevereError( "CmdModifyParameter called with invalid node." );

    SoField* parameterField = m_coinNode->getField( SbName( m_parameterName.toStdString().c_str() ) );
    if( !parameterField )    gf::SevereError( "CmdModifyParameter called with invalid parameterName." );

    SbString fieldValue;
    parameterField->get( fieldValue );
    m_oldValue = QString( fieldValue.getString() );


}

/*!
 * Destroys CmdModifyParameter object.
 */
CmdModifyParameter::~CmdModifyParameter()
{

}

/*!
 * Applies a change to the scene. After undo() the node parameter value will be changed to \a m_oldValue defined in the constructor.
 * \sa undo().
 */
void CmdModifyParameter::undo()
{
    SoField* parameterField = m_coinNode->getField( m_parameterName.toStdString().c_str() );
    if( parameterField )    parameterField->set( m_oldValue.toStdString().c_str() );

}

/*!
 * Applies a change to the scene. After redo() the node parameter value will be changed to \a m_newValue defined in the constructor.
 * \sa undo().
 */
void CmdModifyParameter::redo()
{

    SoField* parameterField = m_coinNode->getField( m_parameterName.toStdString().c_str() );
    if( parameterField )
    {
        parameterField->set( m_newValue.toStdString().c_str() );
    }

}
