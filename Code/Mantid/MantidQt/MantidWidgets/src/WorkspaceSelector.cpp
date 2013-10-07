//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtMantidWidgets/WorkspaceSelector.h"

#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>
#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>

#include "MantidAPI/AlgorithmManager.h"

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

    refresh();
  }
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
      Mantid::API::AnalysisDataService::Instance().isHiddenDataServiceObject(pNf->object_name()) )
  {
    return;
  }

  QString name = QString::fromStdString( pNf->object_name() );
  if ( checkEligibility(name, pNf->object() ) )
  {
    addItem(name);
  }
}

void WorkspaceSelector::handleRemEvent(Mantid::API::WorkspacePostDeleteNotification_ptr pNf)
{
  QString name = QString::fromStdString(pNf->object_name());
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
  QString name = QString::fromStdString(pNf->object_name());
  QString newName = QString::fromStdString(pNf->new_objectname());
  auto& ads = Mantid::API::AnalysisDataService::Instance();

  bool eligible = checkEligibility(newName, ads.retrieve(pNf->new_objectname()));
  int index = findText(name);

  if(eligible)
  {
    if ( index != -1 )
    {
      this->setItemText(index, newName);
    }
    else
    {
      addItem(newName);
    }
  }
  else
  {
    if ( index != -1 )
    {
      removeItem(index);
    }
  }
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
