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
						    bool forScript, const QString & preset_values,
						    const QString & optional_msg,  const QString & enabled_names)
{
  AlgorithmDialog* dlg = NULL;
  if( AlgorithmDialogFactory::Instance().exists(alg->name() + "Dialog") )
  {
    g_log.debug() << "Creating a specialised dialog for " << alg->name() << std::endl;
    dlg = AlgorithmDialogFactory::Instance().createUnwrapped(alg->name() + "Dialog");
    }
  else
  {
    dlg = new GenericDialog;
    g_log.debug() << "No specialised dialog exists for the " << alg->name() 
		  << " algorithm: a generic one has been created" << std::endl;
  }

  // The parent so that the dialog appears on top of it
  dlg->setParent(parent);
  
  // MG 20/07/2009: I have to set the QDialog window flag manually for some reason. 
  //I assumed that the QDialog
  // base class would define this but it doesn't. This should mean that the dialog 
  //will pop up on top of the relevant widget
  Qt::WindowFlags flags = 0;
  flags |= Qt::Dialog;
  flags |= Qt::WindowContextHelpButtonHint;
  dlg->setWindowFlags(flags);


  // Set the content
  dlg->setAlgorithm(alg);
  dlg->setPresetValues(preset_values);
  dlg->isForScript(forScript);
  dlg->setOptionalMessage(optional_msg);
  dlg->setEnabledNames(enabled_names);

  // Setup the layout
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
  try
  {
    user_win = UserSubWindowFactory::Instance().createUnwrapped(iname);
  }
  catch(Mantid::Kernel::Exception::NotFoundError &)
  {
    user_win = NULL;
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
    g_log.error() << "Error creating interface " << iname << "\n";
  }
  return user_win;
}

/**
 * The keys associated with UserSubWindow classes
 * @returns A QStringList containing the keys from the InterfaceFactory that refer to UserSubWindow classes
 */
QStringList InterfaceManagerImpl::getUserSubWindowKeys() const
{
  QStringList key_list;
  std::vector<std::string> keys = UserSubWindowFactory::Instance().getKeys();
  std::vector<std::string>::const_iterator iend = keys.end();
  for( std::vector<std::string>::const_iterator itr = keys.begin(); itr != iend; ++itr )
  {
    key_list.append(QString::fromStdString(*itr));
  }
  return key_list;
}

//----------------------------------
// Private member functions
//----------------------------------
/// Default Constructor
InterfaceManagerImpl::InterfaceManagerImpl()
{
  // Attempt to load libraries that may contain custom interface classes
  const std::string libpath = Mantid::Kernel::ConfigService::Instance().getString("mantidqt.plugins.directory");
  if( !libpath.empty() )
  {
    int nloaded = Mantid::Kernel::LibraryManager::Instance().OpenAllLibraries(libpath);
    if( nloaded == 0 )
    {
      g_log.warning() << "Unable to load Qt plugin libraries.\n"
			  << "Please check that the 'mantidqt.plugins.directory' variable in the .properties file points to "
			  << "the correct location."
			  << std::endl;
    }
  }

}

/// Destructor
InterfaceManagerImpl::~InterfaceManagerImpl()
{
}

