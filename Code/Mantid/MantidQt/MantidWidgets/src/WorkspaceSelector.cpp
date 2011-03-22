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
  m_init(init), m_workspaceTypes(), m_showHidden(true), m_suffix(), m_algName(), m_algPropName(), m_algorithm()
{
  setEditable(false); 
  if( init )
  {
    Mantid::API::AnalysisDataServiceImpl& ads = Mantid::API::AnalysisDataService::Instance();
    ads.notificationCenter.addObserver(m_addObserver);
    ads.notificationCenter.addObserver(m_remObserver);
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

void WorkspaceSelector::showHiddenWorkspaces(const bool & show)
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

QString WorkspaceSelector::getSuffix() const
{
  return m_suffix;
}

void WorkspaceSelector::setSuffix(const QString & suffix)
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
  QString name = QString::fromStdString( pNf->object_name() );
  if ( checkEligibility(name, pNf->object() ) )
  {
    addItem(name);
  }
}

void WorkspaceSelector::handleRemEvent(Mantid::API::WorkspaceDeleteNotification_ptr pNf)
{
  QString name = QString::fromStdString(pNf->object_name());
  int index = findText(name);
  if ( index != -1 )
  {
    removeItem(index);
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
  else if ( ( ! m_showHidden ) && name.startsWith("__") )
  {
    return false;
  }
  else if ( ( ! m_suffix.isEmpty() ) && ( ! name.endsWith(m_suffix) ) )
  {
    return false;
  }

  return true;
}

void WorkspaceSelector::refresh()
{
  clear();
  Mantid::API::AnalysisDataServiceImpl& ads = Mantid::API::AnalysisDataService::Instance();
  std::set<std::string> items = ads.getObjectNames();
  if ( ! items.empty() )
  {
    for ( std::set<std::string>::iterator it = items.begin(); it != items.end(); ++it )
    {      
      QString name = QString::fromStdString(*it);
      if ( checkEligibility( name, ads.retrieve(*it) ) )
      {
        addItem(name);
      }
    }
  }
}