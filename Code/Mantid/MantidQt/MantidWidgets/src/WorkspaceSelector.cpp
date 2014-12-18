//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtMantidWidgets/WorkspaceSelector.h"

#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>

#include "MantidAPI/AlgorithmManager.h"

#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QDebug>
using namespace MantidQt::MantidWidgets;

/**
* Default constructor
* @param parent :: A widget to act as this widget's parent (default = NULL)
* @param init :: If true then the widget will make calls to the framework (default = true)
*/
WorkspaceSelector::WorkspaceSelector(QWidget *parent, bool init) : QComboBox(parent),
  m_addObserver(*this, &WorkspaceSelector::handleAddEvent), 
  m_remObserver(*this, &WorkspaceSelector::handleRemEvent),
  m_clearObserver(*this, &WorkspaceSelector::handleClearEvent),
  m_renameObserver(*this, &WorkspaceSelector::handleRenameEvent),
  m_replaceObserver(*this, &WorkspaceSelector::handleReplaceEvent),
  m_init(init), m_workspaceTypes(), m_showHidden(false), m_optional(false),
  m_suffix(), m_algName(), m_algPropName(), m_algorithm()
{
  setEditable(false); 
  if( init )
  {
    Mantid::API::AnalysisDataServiceImpl& ads = Mantid::API::AnalysisDataService::Instance();
    ads.notificationCenter.addObserver(m_addObserver);
    ads.notificationCenter.addObserver(m_remObserver);
    ads.notificationCenter.addObserver(m_renameObserver);
    ads.notificationCenter.addObserver(m_clearObserver);
    ads.notificationCenter.addObserver(m_replaceObserver);

    refresh();
  }
  this->setAcceptDrops(true);
}

/**
* Destructor for WorkspaceSelector
* De-subscribes this object from the Poco NotificationCentre
*/
WorkspaceSelector::~WorkspaceSelector()
{
  if ( m_init )
  {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_addObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_remObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_clearObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_renameObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
  }
}

QStringList WorkspaceSelector::getWorkspaceTypes() const
{
  return m_workspaceTypes;
}

void WorkspaceSelector::setWorkspaceTypes(const QStringList & types)
{
  if ( types != m_workspaceTypes )
  {
    m_workspaceTypes = types;
    if ( m_init )
    {
      refresh();
    }
  }
}

bool WorkspaceSelector::showHiddenWorkspaces() const
{
  return m_showHidden;
}

void WorkspaceSelector::showHiddenWorkspaces(bool show)
{
  if ( show != m_showHidden )
  {
    m_showHidden = show;
    if ( m_init )
    {
      refresh();
    }
  }
}

bool WorkspaceSelector::isValid() const
{
  return (this->currentText() != "");
}

bool WorkspaceSelector::isOptional() const
{
  return m_optional;
}

void WorkspaceSelector::setOptional(bool optional)
{
  if ( optional != m_optional )
  {
    m_optional = optional;
    if ( m_init ) refresh();
  }
}

QStringList WorkspaceSelector::getSuffixes() const
{
  return m_suffix;
}

void WorkspaceSelector::setSuffixes(const QStringList & suffix)
{
  if ( suffix != m_suffix )
  {
    m_suffix = suffix;
    if ( m_init )
    {
      refresh();
    }
  }
}

QString WorkspaceSelector::getValidatingAlgorithm() const
{
  return m_algName;
}

void WorkspaceSelector::setValidatingAlgorithm(const QString & algName)
{
  if ( algName == m_algName )
  {
    return;
  }
  m_algName = algName;
  if ( m_init )
  {
    m_algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(algName.toStdString());
    m_algorithm->initialize();
    std::vector<Mantid::Kernel::Property*> props = m_algorithm->getProperties();
    for ( std::vector<Mantid::Kernel::Property*>::iterator it = props.begin(); it != props.end(); ++it )
    {
      if ( (*it)->direction() == Mantid::Kernel::Direction::Input )
      {
        // try to cast property to WorkspaceProperty
        Mantid::API::WorkspaceProperty<>* wsProp = dynamic_cast<Mantid::API::WorkspaceProperty<>*>(*it);
        if ( wsProp != NULL )
        {
          m_algPropName = QString::fromStdString((*it)->name());
          break;
        }
      }
    }
    refresh();
  }
}

void WorkspaceSelector::handleAddEvent(Mantid::API::WorkspaceAddNotification_ptr pNf) 
{
  if ( !showHiddenWorkspaces() &&
      Mantid::API::AnalysisDataService::Instance().isHiddenDataServiceObject(pNf->objectName()) )
  {
    return;
  }

  QString name = QString::fromStdString( pNf->objectName() );
  if ( checkEligibility(name, pNf->object() ) )
  {
    addItem(name);
  }
}

void WorkspaceSelector::handleRemEvent(Mantid::API::WorkspacePostDeleteNotification_ptr pNf)
{
  QString name = QString::fromStdString(pNf->objectName());
  int index = findText(name);
  if ( index != -1 )
  {
    removeItem(index);
  }
}

void WorkspaceSelector::handleClearEvent(Mantid::API::ClearADSNotification_ptr)
{
  this->clear();
}

void WorkspaceSelector::handleRenameEvent(Mantid::API::WorkspaceRenameNotification_ptr pNf)
{
  QString name = QString::fromStdString(pNf->objectName());
  QString newName = QString::fromStdString(pNf->newObjectName());
  auto& ads = Mantid::API::AnalysisDataService::Instance();

  bool eligible = checkEligibility(newName, ads.retrieve(pNf->newObjectName()));
  int index = findText(name);
  int newIndex = findText(newName); 
  if(eligible)
  {
    if ( index != -1  && newIndex == -1)
    {
      this->setItemText(index, newName);
    }
    else if (index == -1 && newIndex == -1)
    {
      addItem(newName);
    }else 
      removeItem(index);
  }
  else
  {
    if ( index != -1 )
    {
      removeItem(index);
    }
  }
}

void WorkspaceSelector::handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf)
{

  QString name = QString::fromStdString(pNf->objectName());
  auto& ads = Mantid::API::AnalysisDataService::Instance();

  bool eligible = checkEligibility(name , ads.retrieve(pNf->objectName()));
  int index = findText(name); 

  // if it is inside and it is eligible do nothing
  // if it is not inside and it is eligible insert
  // if it is inside and it is not eligible remove
  // if it is not inside and it is not eligible do nothing
  bool inside = (index != -1); 
  if ( (inside && eligible) || (!inside && !eligible) )
    return;
  else if (!inside && eligible)
    addItem(name);
  else // (inside && !eligible)
    removeItem(index);    
 
}


bool WorkspaceSelector::checkEligibility(const QString & name, Mantid::API::Workspace_sptr object) const
{
  if ( m_algorithm && ! m_algPropName.isEmpty() )
  {
    try
    {
      m_algorithm->setPropertyValue(m_algPropName.toStdString(), name.toStdString());
    }
    catch ( std::invalid_argument & )
    {
      return false;
    }
  }
  else if ( ( ! m_workspaceTypes.empty() ) && m_workspaceTypes.indexOf(QString::fromStdString(object->id())) == -1 )
  {
    return false;
  }
  else if ( ! hasValidSuffix(name) )
  {
    return false;
  }

  return true;
}

bool WorkspaceSelector::hasValidSuffix(const QString& name) const
{
  if(m_suffix.isEmpty())
  {
    return true;
  }
  else
  {
    for (int i =0; i < m_suffix.size(); ++i)
    {
      if(name.endsWith(m_suffix[i]))
      {
        return true;
      }
    }
  }

  return false;
}

void WorkspaceSelector::refresh()
{
  clear();
  if ( m_optional ) addItem("");
  auto& ads = Mantid::API::AnalysisDataService::Instance();
  std::set<std::string> items;
  if ( showHiddenWorkspaces() )
  {
    items = ads.getObjectNamesInclHidden();
  }
  else
  {
    items = ads.getObjectNames();
  }

  for ( std::set<std::string>::iterator it = items.begin(); it != items.end(); ++it )
  {
    QString name = QString::fromStdString(*it);
    if ( checkEligibility( name, ads.retrieve(*it) ) )
    {
      addItem(name);
    }
  }
}

/**
  * Called when an item is dropped
  * @param de :: the drop event data package
  */
void WorkspaceSelector::dropEvent(QDropEvent *de)
{
  const QMimeData *mimeData = de->mimeData(); 
  QString text =  mimeData->text();
  int equal_pos = text.indexOf("=");
  QString ws_name = text.left(equal_pos-1);
  QString ws_name_test = text.mid(equal_pos + 7, equal_pos-1);
  
  if (ws_name == ws_name_test){
    int index = findText(ws_name);
    if (index >= 0){
      setCurrentIndex(index);
      de->acceptProposedAction();
    }
  }
  
}

/**
  * Called when an item is dragged onto a control
  * @param de :: the drag event data package
  */
void WorkspaceSelector::dragEnterEvent(QDragEnterEvent *de)
{
  const QMimeData *mimeData = de->mimeData();  
  if(mimeData->hasText()) 
  {
    QString text = mimeData->text();
    if (text.contains(" = mtd[\""))
      de->acceptProposedAction();
  }
}
