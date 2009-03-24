//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtAPI/InterfaceFactory.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/GenericDialog.h"
#include "MantidQtAPI/UserSubWindow.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/Exception.h"

#include <QStringList>

using namespace MantidQt::API;

//Initialize the logger
Mantid::Kernel::Logger & InterfaceManagerImpl::g_log = Mantid::Kernel::Logger::get("InterfaceManager");

//----------------------------------
// Public member functions
//----------------------------------
/**
 * Return a specialized dialog for the given algorithm. If none exists then the default is returned
 * @param alg A pointer to the algorithm
 * @param parent An optional parent widget
 * @param forScript A boolean indicating if this dialog is to be use for from a script or not
 * @param msg An optional message string to be placed at the top of the dialog
 * @param suggestions An optional set of suggested values
 * @returns An AlgorithmDialog object
 */
AlgorithmDialog* InterfaceManagerImpl::createDialog(Mantid::API::IAlgorithm* alg, QWidget* parent,
						 bool forScript, const QString & msg, const QString & suggestions)
{
  AlgorithmDialog* dlg = NULL;
  if( InterfaceFactory::Instance().exists(alg->name() + "Dialog") )
  {
    g_log.debug() << "Creating a specialised dialog for " << alg->name() << std::endl;
    dlg = qobject_cast<AlgorithmDialog*>(InterfaceFactory::Instance().createUnwrapped(alg->name() + "Dialog"));
    }
  else
  {
    dlg = new GenericDialog(parent);
    g_log.debug() << "No specialised dialog exists for the " << alg->name() 
		  << " algorithm: a generic one has been created" << std::endl;
  }
  
  dlg->setParent(parent);
  dlg->setAlgorithm(alg);
  dlg->setSuggestedValues(suggestions);
  dlg->isForScript(forScript);
  dlg->setOptionalMessage(msg);
  dlg->initializeLayout();
  return dlg;  
}

/**
 * Create a new instance of the correct type of UserSubWindow
 * @param interface_name The registered name of the interface
 * @param parent The parent widget
 */
UserSubWindow* InterfaceManagerImpl::createSubWindow(const QString & interface_name, QWidget* parent)
{
  UserSubWindow *user_win = NULL;
  std::string iname = interface_name.toStdString();
  if( InterfaceFactory::Instance().exists(iname) )
  {
    user_win = qobject_cast<UserSubWindow*>(InterfaceFactory::Instance().createUnwrapped(iname));
  }
  if( user_win )
  {
    g_log.debug() << "Created a specialised interface for " << iname << std::endl;
    user_win->setParent(parent);
    user_win->setInterfaceName(interface_name);
    user_win->initializeLayout();    
  }
  else 
  {
    g_log.debug() << "No specialised interface exists for " << iname << std::endl;
  }
  return user_win;
}

/**
 * The keys associated with UserSubWindow classes
 * @returns A QStringList containing the keys from the InterfaceFactory that refer to UserSubWindow classes
 */
QStringList InterfaceManagerImpl::getUserSubWindowKeys() const
{
  QStringList m_key_list;
  std::vector<std::string> keys = InterfaceFactory::Instance().getKeys();
  std::vector<std::string>::const_iterator iend = keys.end();
  for( std::vector<std::string>::const_iterator itr = keys.begin(); itr != iend; ++itr )
  {
    QString key = QString::fromStdString(*itr);
    if( !key.endsWith("Dialog") ) m_key_list.append(key);
  }
  return m_key_list;
}

//----------------------------------
// Private member functions
//----------------------------------
/// Default Constructor
InterfaceManagerImpl::InterfaceManagerImpl()
{

  //Attempt to load libraries that may contain custom dialog classes
  //First get the path from the configuration manager
  std::string libpath = Mantid::Kernel::ConfigService::Instance().getString("plugins.directory");
  if( !libpath.empty() )
  {
    int loaded = Mantid::Kernel::LibraryManager::Instance().OpenAllLibraries(libpath);
    if( loaded == 0 )
    {
      g_log.information() << "A path has been specified for the custom algorithm dialogs but no libraries could be loaded. "
			  << "Please check that the 'plugins.directory' variable in the Mantid.properties file points to "
			  << "the correct location."
			  << std::endl;
    }
  }

}

/// Destructor
InterfaceManagerImpl::~InterfaceManagerImpl()
{
}

