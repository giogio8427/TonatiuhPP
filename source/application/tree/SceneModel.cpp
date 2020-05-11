#include <QIcon>
#include <QMessageBox>

#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/nodekits/SoSceneKit.h>
#include <Inventor/nodekits/SoNodeKitListPart.h>

#include "libraries/geometry/gf.h"

#include "tree/SoPathVariant.h"
#include "SceneModel.h"
#include "kernel/run/InstanceNode.h"
#include "kernel/material/MaterialAbstract.h"
#include "kernel/scene/TSceneKit.h"
#include "kernel/scene/TSeparatorKit.h"
#include "kernel/scene/TShapeKit.h"
#include "kernel/shape/ShapeAbstract.h"
#include "kernel/sun/TLightKit.h"
//#include "kernel/trackers/TSceneTracker.h"
#include "kernel/trackers/TrackerAbstract.h"

/*!
 * Creates an empty model.
 */
SceneModel::SceneModel(QObject* parent):
    QAbstractItemModel(parent), m_coinRoot(0), m_coinScene(0), m_instanceRoot(0)
{

}

/*!
 * Destroys the model.
 */
SceneModel::~SceneModel()
{
    Clear();
}

/*!
 * Sets \a coinRoot as the model root.
 */
void SceneModel::SetCoinRoot(SoSeparator& coinRoot)
{
    m_coinRoot = &coinRoot;
    m_mapCoinQt.clear();
}

/*!
 * Sets the model scene to the given \a coinScene.
 *
 * Creates nodes for the model to the scene subnodes.
 */
void SceneModel::SetCoinScene(TSceneKit& coinScene)
{
    beginResetModel();
    if (m_instanceRoot) Clear();
    m_mapCoinQt.clear();
    m_coinScene = 0;
    m_coinScene = &coinScene;

    SetRoot();
    SetLight();
    SetConcentrator();
    endResetModel();
}

/*!
 * Clears the model scene
 *
 * Clears nodes from the model.
 */
void SceneModel::Clear()
{
    DeleteInstanceTree(*m_instanceRoot);

    delete m_instanceRoot;
    m_instanceRoot = 0;
}

/*!
 * Sets model root
 *
 * Sets \a m_coinScene as model.
 */
void SceneModel::SetRoot()
{
    m_instanceRoot = new InstanceNode(m_coinScene);
    QList< InstanceNode* > instanceRootNodeList;
    m_mapCoinQt.insert(std::make_pair(m_coinScene, instanceRootNodeList) );
    m_mapCoinQt[m_coinScene].append(m_instanceRoot);
}

void SceneModel::SetLight()
{
    if ( m_coinScene->getPart("lightList[0]", false ) )
    {
        TLightKit* tlightKit = static_cast< TLightKit* >( m_coinScene->getPart("lightList[0]", false ) );
        InsertLightNode( *tlightKit );
    }
}

void SceneModel::SetConcentrator()
{
    SoNodeKitListPart* coinPartList = static_cast<SoNodeKitListPart*>( m_coinScene->getPart( "childList", true ) );

    //TSeparatorKit* separatorKit;
    if (coinPartList && coinPartList->getNumChildren() == 0)
    {
        //Create Sun coordinate system node
        TSeparatorKit* nodeSun = new TSeparatorKit();
        nodeSun->setName("SunNode");
        coinPartList->addChild(nodeSun);
        nodeSun->setSearchingChildren(true);
        InstanceNode* instanceNode = AddInstanceNode( *m_instanceRoot, nodeSun );
        SoNodeKitListPart* nodeSunChild = static_cast< SoNodeKitListPart* >( nodeSun->getPart( "childList", true ) );

//        TSceneTracker* sceneTracker = new TSceneTracker;
//        nodeSun->setPart("tracker", sceneTracker);
//        sceneTracker->SetSceneKit(m_coinScene);

        //Create Concentrator coordinate system node
        TSeparatorKit* nodeRoot = new TSeparatorKit();
        nodeRoot->setName("RootNode");
        nodeSunChild->addChild(nodeRoot);
        nodeRoot->setSearchingChildren(true);

        m_instanceConcentrator = AddInstanceNode( *instanceNode, nodeRoot );

        //
//        TLightKit* lightKit = dynamic_cast<TLightKit*>(m_coinScene->getPart("lightList[0]", false));
//        if (lightKit) {
//            sceneTracker->SetAzimuthAngle(&lightKit->azimuth);
//            sceneTracker->SetZenithAngle(&lightKit->zenith);
//        }
    }
    else
    {
        TSeparatorKit* sunSeparatorKit = static_cast< TSeparatorKit* >( coinPartList->getChild( 0 ) );
        if (!sunSeparatorKit) return;

        InstanceNode* sunInstanceNode = AddInstanceNode( *m_instanceRoot, sunSeparatorKit );
        SoNodeKitListPart* sunSeparatorChildList = static_cast< SoNodeKitListPart* >( sunSeparatorKit->getPart( "childList", true ) );
        if (!sunSeparatorChildList) return;

        SoNode* tracker = sunSeparatorKit->getPart( "tracker", false );
        if (tracker)
        {
            TrackerAbstract* trackerNode = static_cast< TrackerAbstract* >( tracker );
            trackerNode->SetSceneKit(m_coinScene);
        }

        TSeparatorKit* separatorKit = static_cast< TSeparatorKit* >( sunSeparatorChildList->getChild( 0 ) );
        if (!separatorKit) return;

        m_instanceConcentrator = AddInstanceNode( *sunInstanceNode, separatorKit );
        GenerateInstanceTree( *m_instanceConcentrator );
    }
}

InstanceNode* SceneModel::AddInstanceNode( InstanceNode& instanceNodeParent, SoNode* soNode )
{
    InstanceNode* instanceNode = new InstanceNode( soNode );
    if( instanceNode )
    {
        instanceNodeParent.AddChild( instanceNode );
        QList< InstanceNode* > instanceNodeList;
        m_mapCoinQt.insert( std::make_pair( soNode, instanceNodeList ) );
        m_mapCoinQt[soNode].append( instanceNode );
    }
    return instanceNode;
}

void SceneModel::GenerateInstanceTree( InstanceNode& instanceNodeParent )
{
    SoNode* parentNode = instanceNodeParent.GetNode();

    if( parentNode )
    {
        if( parentNode->getTypeId().isDerivedFrom( SoBaseKit::getClassTypeId() ) )
        {
            if( parentNode->getTypeId().isDerivedFrom( TShapeKit::getClassTypeId() ) )
            {
                GenerateTShapeKitSubTree( instanceNodeParent, parentNode  );
            }
            else
            {
                GenerateTSeparatorKitSubTree( instanceNodeParent, parentNode  );
            }
        }
    }
}

void SceneModel::GenerateTShapeKitSubTree( InstanceNode& instanceNodeParent, SoNode* parentNode )
{
    TShapeKit* parentKit = static_cast< TShapeKit* >( parentNode );
    if( parentKit )
    {
        SoNode* shape = parentKit->getPart("shape", false);
        if( shape )    AddInstanceNode( instanceNodeParent, shape );
        SoNode* material = parentKit->getPart("appearance.material", false);
        if( material ) AddInstanceNode( instanceNodeParent, material );
    }
}

void SceneModel::GenerateTSeparatorKitSubTree( InstanceNode& instanceNodeParent, SoNode* parentNode )
{
    //Is TSeparatorKit node
    SoBaseKit* parentKit = static_cast< SoBaseKit* >( parentNode );
    if( parentKit )
    {
        SoNode* tracker = parentKit->getPart( "tracker", false );
        if( tracker )
        {
            TrackerAbstract* trackerNode = static_cast< TrackerAbstract* >( tracker );
            trackerNode->SetSceneKit( m_coinScene );
            AddInstanceNode( instanceNodeParent, tracker );
        }

        SoNodeKitListPart* coinPartList = static_cast< SoNodeKitListPart* >( parentKit->getPart( "childList", false ) );
        if (coinPartList)
        {
            for( int index = 0; index < coinPartList->getNumChildren(); ++index )
            {
                SoBaseKit* coinChild = static_cast< SoBaseKit* >( coinPartList->getChild( index ) );
                InstanceNode* instanceChild = AddInstanceNode( instanceNodeParent, coinChild );
                if( instanceChild) GenerateInstanceTree( *instanceChild );
            }
        }
    }
}

QModelIndex SceneModel::index(int row, int column, const QModelIndex& parentModelIndex) const
{
    if (!m_instanceRoot) return QModelIndex();
    InstanceNode* instanceParent = NodeFromIndex(parentModelIndex);
    return createIndex(row, column, instanceParent->children[row]);
}

InstanceNode* SceneModel::NodeFromIndex(const QModelIndex& modelIndex) const
{
    if ( modelIndex.isValid() ) return static_cast< InstanceNode* >( modelIndex.internalPointer() );
    else return m_instanceRoot;
}

int SceneModel::rowCount(const QModelIndex& parentModelIndex) const
{
    InstanceNode* instanceParent = NodeFromIndex(parentModelIndex);
    return instanceParent ? instanceParent->children.count() : 0;
}

int SceneModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QModelIndex SceneModel::parent(const QModelIndex& index) const
{
    InstanceNode* instanceChild = NodeFromIndex(index);
    if (!instanceChild) return QModelIndex();

    InstanceNode* instanceParent = instanceChild->GetParent();
    if (!instanceParent) return QModelIndex();

    InstanceNode* instanceGrandparent = instanceParent->GetParent();
    if (!instanceGrandparent) return QModelIndex();

    int row = instanceGrandparent->children.indexOf(instanceParent);
    return createIndex(row, 0, instanceParent);
//    return createIndex(row, childModelIndex.column(), instanceParent);
}

QVariant SceneModel::headerData(int section, Qt::Orientation orientation, int role ) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            if (section == 0) return "Node";
        }
    }
    return QVariant();
}

QVariant SceneModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::UserRole && role != Qt::DecorationRole) return QVariant();

    InstanceNode* instanceNode = NodeFromIndex(index);
    if (!instanceNode) return QVariant();

    SoNode* nodeCoin = instanceNode->GetNode();
    if (!nodeCoin) return QVariant();

    if (index.column() == 0)
    {
        if (role == Qt::DisplayRole)
        {
            QString name;
            if ( nodeCoin->getName() == SbName() )
                name = nodeCoin->getTypeId().getName().getString();
            else
                name = nodeCoin->getName().getString();

            SoSearchAction action;
            action.setNode(nodeCoin);
            action.setInterest(SoSearchAction::ALL);
            action.apply(m_coinRoot);

            int count = action.getPaths().getLength();
            if (count > 1)
                name = QString("%1 [%2]").arg(name).arg(count);

            return name;
        }
        if (role == Qt::UserRole)
        {
            SoNodeKitPath* coinKitPath = PathFromIndex(index);
            if (coinKitPath)
            {
                SoPathVariant pathWrapper(coinKitPath);
                return QVariant::fromValue(pathWrapper);
            }
            coinKitPath->unref();
            return  QVariant();
        }
        if (role == Qt::DecorationRole)
        {
            if (nodeCoin->getTypeId().isDerivedFrom( TLightKit::getClassTypeId() ) )
            {
                return QIcon(":/images/scene/environmentSun.png");
            }
            else if (nodeCoin->getTypeId().isDerivedFrom(TSeparatorKit::getClassTypeId() ) )
            {
                TSeparatorKit* separatorKit = static_cast<TSeparatorKit*>( nodeCoin );
                return QIcon( separatorKit->getIcon() );
            }
            else if (nodeCoin->getTypeId().isDerivedFrom(TShapeKit::getClassTypeId() ) )
            {
//                SoBaseKit* nodeKit = static_cast< SoBaseKit* >( coinNode );
//                TShape* kit = static_cast<TShape*>( nodeKit->getPart( "shape", false ) );
//                if( kit ) return QIcon( kit->GetIcon() );
                return QIcon(":/images/scene/nodeShape.png");
            }
            else if (nodeCoin->getTypeId().isDerivedFrom(SoShape::getClassTypeId() ) )
            {
                ShapeAbstract* shape = static_cast<ShapeAbstract*>(nodeCoin);
                return QIcon(shape->getTypeIcon());
            }
            else if (nodeCoin->getTypeId().isDerivedFrom(MaterialAbstract::getClassTypeId() ) )
            {
                MaterialAbstract* material = static_cast<MaterialAbstract*>( nodeCoin );
                return QIcon(material->getTypeIcon());
            }
            else if (nodeCoin->getTypeId().isDerivedFrom(TrackerAbstract::getClassTypeId() ) )
            {
                TrackerAbstract* tracker = static_cast<TrackerAbstract*>(nodeCoin);
                return QIcon(tracker->getTypeIcon());
            }
         }
    }

    return QVariant();
}

int SceneModel::InsertCoinNode( SoNode& coinChild, SoBaseKit& coinParent )
{
    int row = -1;
    if (coinParent.getTypeId().isDerivedFrom( TSeparatorKit::getClassTypeId() ) )
    {
        if( coinChild.getTypeId().isDerivedFrom( TrackerAbstract::getClassTypeId() ) )
        {
            coinParent.setPart( "tracker", &coinChild );
            row = 0;
        }
        else
        {
            SoNodeKitListPart* coinPartList = static_cast< SoNodeKitListPart* >( coinParent.getPart( "childList", true ) );
            if ( !coinPartList ) return -1;

            row = coinPartList->getNumChildren();

            if( coinChild.getTypeId().isDerivedFrom( SoBaseKit::getClassTypeId() ) )
            {
                SoBaseKit* childKit = static_cast< SoBaseKit* >( &coinChild );
                coinPartList->addChild( childKit );
                childKit->setSearchingChildren( true );

                if( coinParent.getPart( "tracker", false ) ) row++;
            }
        }
    }
    else
    {
        TShapeKit* shapeKit = static_cast< TShapeKit* > ( &coinParent );
        if( shapeKit->getPart( "shape", false ) ) row++;
        if( shapeKit->getPart( "appearance.material", false ) ) row++;
    }

    QList<InstanceNode*>& instanceListParent = m_mapCoinQt[ &coinParent ];
    for ( int index = 0; index < instanceListParent.count(); index++ )
    {
        InstanceNode* instanceParent = instanceListParent[index];
        InstanceNode* instanceChild = new InstanceNode( &coinChild );
        instanceParent->InsertChild( row, instanceChild );

        //Inserting InstanceNode in the map
        QList< InstanceNode* > instanceNodeList;
            m_mapCoinQt.insert( std::make_pair( &coinChild, instanceNodeList ) );
        m_mapCoinQt[ &coinChild ].append( instanceChild );

        GenerateInstanceTree( *instanceChild );
    }
    emit layoutChanged();

    return row;
}

/*!
 * Insert a light node to the model. If the model has an other light node, the previous node
 * will be deleted.
 */
void SceneModel::InsertLightNode(TLightKit& lightKit)
{
    SoNodeKitListPart* lightList =
            static_cast<SoNodeKitListPart*>(m_coinScene->getPart("lightList", true) ) ;
    if (lightList->getNumChildren() > 0)
        if (m_instanceRoot->children.size() > 0)
            m_instanceRoot->children.remove(0);

    m_coinScene->setPart("lightList[0]", &lightKit);

    SoSearchAction trackersSearch;
    trackersSearch.setType( TrackerAbstract::getClassTypeId() );
    trackersSearch.setInterest( SoSearchAction::ALL);
    trackersSearch.apply( m_coinRoot );
    SoPathList& trackersPath = trackersSearch.getPaths();

    for( int index = 0; index < trackersPath.getLength(); ++index )
    {
        SoFullPath* trackerPath = static_cast< SoFullPath* > ( trackersPath[index] );
        TrackerAbstract* tracker = static_cast< TrackerAbstract* >( trackerPath->getTail() );
//        tracker->SetAzimuthAngle(&lightKit.azimuth);
//        tracker->SetZenithAngle(&lightKit.zenith);
    }

    InstanceNode* instanceLight = new InstanceNode(&lightKit);
    m_instanceRoot->InsertChild(0, instanceLight);

    emit LightNodeStateChanged(1);
    emit layoutChanged();
}


void SceneModel::RemoveCoinNode( int row, SoBaseKit& coinParent )
{
    if (coinParent.getTypeId().isDerivedFrom( TSeparatorKit::getClassTypeId() ) )
    {
        if( ( row == 0 ) && coinParent.getPart( "tracker", false ) )
        {
            coinParent.setPart( "tracker", NULL );
        }
        else
        {
            SoNodeKitListPart* coinPartList = static_cast< SoNodeKitListPart* >( coinParent.getPart( "childList", false ) );
            if ( coinPartList )
            {
                if( coinParent.getPart( "tracker", false ) )    coinPartList->removeChild( row - 1 );
                else coinPartList->removeChild( row );
            }

        }

    }

    QList<InstanceNode*> instanceListParent = m_mapCoinQt[ &coinParent ];

    for( int index = 0; index< instanceListParent.size(); ++index )
    {
        InstanceNode* instanceParent = instanceListParent[index];
        InstanceNode* instanceNode = instanceParent->children[row];
        instanceParent->children.remove(row);

        QList<InstanceNode*>& instanceList = m_mapCoinQt[ instanceNode->GetNode()];
        instanceList.removeAt( instanceList.indexOf( instanceNode ) );
    }
    emit layoutChanged();
}

void SceneModel::RemoveLightNode(TLightKit& coinLight)
{
    SoNodeKitListPart* lightList = static_cast< SoNodeKitListPart* >( m_coinScene->getPart( "lightList", true ) );
    if ( lightList ) lightList->removeChild( &coinLight );
    m_instanceRoot->children.remove( 0 );

    SoSearchAction trackersSearch;
    trackersSearch.setType( TrackerAbstract::getClassTypeId() );
    trackersSearch.setInterest( SoSearchAction::ALL);
    trackersSearch.apply( m_coinRoot );
    SoPathList& trackersPath = trackersSearch.getPaths();

    for( int index = 0; index <trackersPath.getLength(); ++index )
    {
        SoFullPath* trackerPath = static_cast< SoFullPath* > ( trackersPath[index] );
        TrackerAbstract* tracker = dynamic_cast< TrackerAbstract* >( trackerPath->getTail() );
        tracker->Disconnect();
    }

    emit LightNodeStateChanged( 0 );
    emit layoutChanged();
}

void SceneModel::ReconnectAllTrackers()
{
    TLightKit * coinLight =  static_cast< TLightKit* >( m_coinScene->getPart( "lightList[0]", false ));
    m_instanceRoot->ReconnectAllTrackers(coinLight);
}

void SceneModel::DisconnectAllTrackers()
{
    m_instanceRoot->DisconnectAllTrackers();
}

Qt::ItemFlags SceneModel::flags( const QModelIndex& modelIndex ) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( modelIndex );

    if ( modelIndex.isValid() )
    {
        if ( modelIndex.column() > 0 ) return defaultFlags;
        else return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable | defaultFlags;
    }
    else return defaultFlags;
}

Qt::DropActions SceneModel::supportedDropActions() const
{
    return  Qt::CopyAction | Qt::MoveAction;
}

Qt::DropActions SceneModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}
/**
 * Cuts the \a row child from \a coinParent node.
 *
 * Returns whether the cut is successfully done.
**/
bool SceneModel::Cut(SoBaseKit& coinParent, int row)
{
    if (row < 0) return false;

    QList<InstanceNode*> instanceListParent = m_mapCoinQt[&coinParent];
    InstanceNode* instanceParent = instanceListParent[0];

    SoNode* coinChild = instanceParent->children[row]->GetNode();

    QModelIndex parentIndex = createIndex(instanceParent->GetParent()->children.indexOf(instanceParent), 0, instanceParent);
    beginRemoveRows(parentIndex, row, row);

    if( !coinChild->getTypeId().isDerivedFrom( SoBaseKit::getClassTypeId() ) )
    {
        SbString partName = coinParent.getPartString( coinChild );
        if( partName.getLength() == 0 ) partName = "material";

        coinParent.setPart( partName, NULL );
    }
    else
    {
        if( coinParent.getTypeId().isDerivedFrom( TSeparatorKit::getClassTypeId() ) )
        {

            SoNodeKitListPart* coinPartList = static_cast< SoNodeKitListPart* >( coinParent.getPart( "childList", false ) );
            if( !coinPartList ) return false;


            if( coinParent.getPart( "tracker", false ) )
            {
                coinChild = coinPartList->getChild( row-1 );
                coinPartList->removeChild( row -1 );
            }
            else
            {
                coinChild = coinPartList->getChild( row );
                coinPartList->removeChild( row );
            }

            if( !coinChild ) gf::SevereError( "SceneModel::Cut Null coinChild pointer" );

        }
    }

    QList<InstanceNode*> instanceList = m_mapCoinQt[ coinChild ];

    // Test if the node is shared
    if( instanceList.size() == 1 )
    {
        InstanceNode* instanceNode = instanceList[0];
        DeleteInstanceTree( *instanceNode );
        //delete instanceNode;
        //instanceNode = 0;
    }
    else
    {
        QList<InstanceNode*> instanceListParent = m_mapCoinQt[ &coinParent ];
        if( instanceListParent.size() == 1 )
        {
            InstanceNode* instanceParent = instanceListParent[0];
            InstanceNode* instanceNode = instanceParent->children[row];
            DeleteInstanceTree( *instanceNode );
            //delete instanceNode;
            //instanceNode = 0;
        }
        else
        {
            for( int index = 0; index< instanceListParent.size(); ++index )
            {
                InstanceNode* instanceParent = instanceListParent[index];
                InstanceNode* instanceNode = instanceParent->children[row];
                DeleteInstanceTree( *instanceNode );
                //delete instanceNode;
                //instanceNode = 0;
            }
        }
    }

    endRemoveRows();
    emit layoutChanged();

    return true;
}


/**
 * Returns the index of item with the \a nodeUrl.
 *
 * If \a nodeUrl is not a valid node url, the function returns root node index.
 *
 *
 * \sa IndexFromNodeUrl, NodeFromIndex, PathFromIndex.
**/
QModelIndex SceneModel::IndexFromNodeUrl( QString nodeUrl ) const
{
    QStringList nodeList = nodeUrl.split( QLatin1String( "/" ), QString::SkipEmptyParts );

    if( ( nodeList.size() == 1 ) &&
            ( nodeList[0] == QLatin1String( "Light" ) ) )    return index( 0, 0 );

    if( nodeList.size() > 0 )
    {
        QString nodeName = nodeList.last();
        nodeList.removeLast();

        QString parentNodeURL = QString( QLatin1String( "//" ) ) + nodeList.join( QLatin1String( "/" ) );
        QModelIndex parentIndex = IndexFromNodeUrl( parentNodeURL );
        InstanceNode* parentNode = NodeFromIndex( parentIndex );

        int child = 0;
        int row = -1;
        while( child < parentNode->children.count() )
        {
            if( parentNode->children[child]->GetNode()->getName()  == SbName( nodeName.toStdString().c_str() ) )
            {
                row = child;
                break;
            }
            child++;
        }
        if( row < 0 )    return QModelIndex();
        //if(nodeName == ".") return index(row+1, 0, parentIndex );
        return index(row, 0, parentIndex );
    }
    else
        return QModelIndex();
}

/**
 * Returns the index of item with the \a coinNodePath.
 *
 * If \a coinNodePath is not a valid node path, the function returns root node index.
 *
 *
 * \sa IndexFromNodeUrl, NodeFromIndex, PathFromIndex.
**/
QModelIndex SceneModel::IndexFromPath( const SoNodeKitPath& coinNodePath ) const
{
    SoBaseKit* coinNode = static_cast< SoBaseKit* >( coinNodePath.getTail() );
    if( !coinNode ) gf::SevereError( "IndexFromPath Null coinNode." );

    if( coinNode->getTypeId().getName().getString() == SbName("TLightKit") ) return index( 0, 0 );

    if ( coinNodePath.getLength() > 1 )
    {
        SoBaseKit* coinParent = static_cast< SoBaseKit* >( coinNodePath.getNodeFromTail(1) );
        if( !coinParent ) gf::SevereError( "IndexFromPath Null coinParent." );

        if( coinParent->getTypeId().getName() == SbName( "TLightKit") ) return index( 0, 0 );

        SoNodeKitListPart* coinPartList = static_cast< SoNodeKitListPart* >( coinParent->getPart( "childList", true ) );
        if( !coinPartList ) gf::SevereError( "IndexFromPath Null coinPartList." );

        int row = coinPartList->findChild(coinNode);
        if( coinParent->getTypeId().isDerivedFrom( TSeparatorKit::getClassTypeId() ) )
        {
            if (coinParent->getPart("tracker", false)) row++;
        }
        if( coinNodePath.getNodeFromTail(1)->getTypeId().isDerivedFrom( SoSceneKit::getClassTypeId() ) )
        {
            int child = 0;
            while( child < m_instanceRoot->children.count() )
            {
                if ( m_instanceRoot->children[child]->GetNode() == coinNode )
                    break;
                child++;
            }
            return index( child, 0 );
        }
        else
        {
            SoNodeKitPath* parentPath = static_cast< SoNodeKitPath* > ( coinNodePath.copy( 0, 0 ) );
            parentPath->truncate( parentPath->getLength() - 1 );
                        QModelIndex parentIndex = IndexFromPath( *parentPath );
            QModelIndex childIndex =  index (row, 0, parentIndex );
            return childIndex;
        }

    }
    else
        return QModelIndex();
}

/**
 * Adds a child to \a coinParent as \a row child. If \a type is tgc::Copied, the added child is a copy of \a coinNode. But the type is tgc::Shared the node is shared with previous parent.
 * Return true if the paste action is correctly done. Otherwise returns false.
**/
bool SceneModel::Paste( tgc::PasteType type, SoBaseKit& coinParent, SoNode& coinNode, int row )
{
    SoNode* coinChild = 0;
    SoNode* pCoinParent = 0;
    pCoinParent = &coinParent;

    if (type == tgc::Shared) coinChild = &coinNode;

    switch (type)
    {
        case tgc::Copied :
            coinChild = static_cast< SoNode* >( coinNode.copy( true ) );
            break;

        case tgc::Shared :
        default :
            coinChild = &coinNode;
            break;
    }
    if ( !coinChild->getTypeId().isDerivedFrom( SoBaseKit::getClassTypeId() ) )
    {
        if ( coinChild->getTypeId().isDerivedFrom( TrackerAbstract::getClassTypeId() ) )
        {
            TSeparatorKit* separatorKit = static_cast< TSeparatorKit* >( pCoinParent );
            TrackerAbstract* tracker = static_cast< TrackerAbstract* >( separatorKit->getPart( "tracker", false ) );
            if (tracker)
            {
                QMessageBox::warning( 0, tr( "Tonatiuh warning" ), tr( "This TSeparatorKit already contains a tracker" ) );
                return false;
            }
            coinParent.setPart( "tracker", coinChild );

        }
        if ( coinChild->getTypeId().isDerivedFrom( SoShape::getClassTypeId() ) )
        {
            TShapeKit* shapeKit = static_cast< TShapeKit* >( pCoinParent );
            if(!shapeKit)    return false;
            ShapeAbstract* shape = static_cast< ShapeAbstract* >( shapeKit->getPart( "shape", false ) );

            if (shape)
            {
                QMessageBox::warning( 0, tr( "Tonatiuh warning" ), tr( "This TShapeKit already contains a shape" ) );
                return false;
            }
            coinParent.setPart("shape", coinChild );
        }
        if ( coinChild->getTypeId().isDerivedFrom( SoMaterial::getClassTypeId() ) )
        {
            TShapeKit* shapeKit = static_cast< TShapeKit* >( pCoinParent );
            if(!shapeKit)    return false;
            MaterialAbstract* material = static_cast< MaterialAbstract* >( shapeKit->getPart( "material", false ) );
            if (material)
            {
                QMessageBox::warning( 0, tr( "Tonatiuh warning" ), tr( "This TShapeKit already contains a material" ) );
                return false;
            }
            coinParent.setPart("material", coinChild );
        }

    }
    else
    {
        SoNodeKitListPart* coinPartList = static_cast< SoNodeKitListPart* >( coinParent.getPart( "childList", true ) );
        if( !coinPartList ) gf::SevereError( "SceneModel::Paste Null coinPartList pointer" );
        if( coinParent.getPart( "tracker", false ) ) coinPartList->insertChild( coinChild, row -1 );
        else coinPartList->insertChild( coinChild, row );
    }

    QList<InstanceNode*>& instanceListParent = m_mapCoinQt[ &coinParent ];
    for ( int index = 0; index < instanceListParent.count(); ++index )
    {
        InstanceNode* instanceParent = instanceListParent[index];
        InstanceNode* instanceChild = new InstanceNode( coinChild );
        instanceParent->InsertChild( row, instanceChild );

        //Inserting InstanceNode in the map
        QList< InstanceNode* > instanceNodeList;
            m_mapCoinQt.insert( std::make_pair( coinChild, instanceNodeList ) );
        m_mapCoinQt[ coinChild ].append( instanceChild );
        GenerateInstanceTree( *instanceChild );
    }

    emit layoutChanged();
    return true;
}


bool SceneModel::SetNodeName( SoNode* coinChild, QString newName )
{
    QList< InstanceNode* > nodeInstances = m_mapCoinQt[ coinChild ];
    if( nodeInstances.size() == 0 ) return false;

    QList< InstanceNode* > instancesParent;
    for (int index = 0; index < nodeInstances.size(); ++index )
    {
        InstanceNode* instance = nodeInstances[index];
        QVector< InstanceNode* > parentChildren = instance->GetParent()->children;
        int childIndex = parentChildren.indexOf( instance );
        for( int child = 0; child < parentChildren.size(); ++child )
        {

            SbName idChildName = parentChildren[child]->GetNode()->getName();
            if( child!= childIndex && idChildName == newName.toStdString().c_str() )    return false;
        }
    }
    coinChild->setName( newName.toStdString().c_str() );

    emit layoutChanged();
    return true;
}

void SceneModel::UpdateSceneModel()
{
    /*TLightKit* lightKit = static_cast< TLightKit* >( m_coinScene->getPart( "lightList[0]", false ) );
    if ( !lightKit )    return;

    TSeparatorKit* concentratorRoot = static_cast< TSeparatorKit* >( m_coinScene->getPart( "childList[0]", false ) );
    if ( !concentratorRoot )    return;

    SoGetBoundingBoxAction* bbAction = new SoGetBoundingBoxAction( SbViewportRegion() ) ;
    concentratorRoot->getBoundingBox( bbAction );

    SbBox3f box = bbAction->getXfBoundingBox().project();
    delete bbAction;

    BBox sceneBox;
    if( !box.isEmpty() )
    {
        sceneBox.pMin = Point3D( box.getMin()[0], box.getMin()[1], box.getMin()[2] );
        sceneBox.pMax = Point3D( box.getMax()[0], box.getMax()[1], box.getMax()[2] );
        if( lightKit ) lightKit->Update( sceneBox );
    }
    */
    emit layoutChanged();
}

void SceneModel::DeleteInstanceTree(InstanceNode& instanceNode)
{
    while (instanceNode.children.count()>0)
    {
        InstanceNode* childInstance = instanceNode.children[instanceNode.children.count()-1];
        DeleteInstanceTree( *childInstance );
        delete childInstance;
        childInstance = 0;
    }

    QList<InstanceNode*>& instanceList = m_mapCoinQt[ instanceNode.GetNode()];
    instanceList.removeAt( instanceList.indexOf( &instanceNode ) );

    InstanceNode* instanceParent = instanceNode.GetParent();
    if( instanceParent )
    {
        int row = instanceParent->children.indexOf( &instanceNode );
        instanceParent->children.remove( row );
    }
}

SoNodeKitPath* SceneModel::PathFromIndex( const QModelIndex& modelIndex ) const
{
    SoNode* coinNode = NodeFromIndex( modelIndex )->GetNode();
    if (!coinNode->getTypeId().isDerivedFrom( SoBaseKit::getClassTypeId() ) )
    {
        QModelIndex parentIndex = parent( modelIndex );
        SoNodeKitPath* parentPath = PathFromIndex( parentIndex );
        return parentPath;
    }
    else
    {
        SoSearchAction coinSearch;
        coinSearch.setNode( m_coinScene );
        coinSearch.setInterest( SoSearchAction::FIRST);
        coinSearch.apply( m_coinRoot );

        SoPath* coinScenePath = coinSearch.getPath( );
        if( !coinScenePath ) gf::SevereError( "PathFromIndex Null coinScenePath." );

        SoNodeKitPath* nodePath = static_cast< SoNodeKitPath* > ( coinScenePath );
        if( !nodePath ) gf::SevereError( "PathFromIndex Null nodePath." );

        nodePath->ref();

        InstanceNode* instanceNode = NodeFromIndex( modelIndex );

        SoNodeList nodeList;
        while(instanceNode->GetParent() )
        {
            nodeList.append( instanceNode->GetNode() );
            instanceNode = instanceNode->GetParent();
        }

        for( int i = nodeList.getLength(); i > 0; --i )
        {
            SoBaseKit* coinNode = static_cast< SoBaseKit* >( nodeList[i-1] );
            if ( coinNode )    nodePath->append( coinNode );

        }
        return nodePath;
    }
}
