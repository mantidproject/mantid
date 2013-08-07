//----------------------------------
// Includes
//----------------------------------
#include <Poco/Environment.h>
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtAPI/InterfaceFactory.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/GenericDialog.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtAPI/VatesViewerInterface.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Exception.h"

#include <QStringList>

using namespace MantidQt::API;

//Initialize the logger
Mantid::Kernel::Logger & InterfaceManager::g_log = Mantid::Kernel::Logger::get("InterfaceManager");

Mantid::Kernel::AbstractInstantiator<VatesViewerInterface> *InterfaceManager::m_vatesGuiFactory = NULL;

//----------------------------------
// Public member functions
//----------------------------------
/**
 * Return a specialized dialog for the given algorithm. If none exists then the default is returned
 * @param alg :: A pointer to the algorithm
 * @param parent :: An optional parent widget
 * @param forScript :: A boolean indicating if this dialog is to be use for from a script or not
 * @param preset_values :: TODO: Write description of this variable.
 * @param optional_msg :: An optional message string to be placed at the top of the dialog
 * @param enabled :: TODO: Write description of this variable.
 * @param disabled :: TODO: Write description of this variable. 
 * @returns An AlgorithmDialog object
 */
AlgorithmDialog* InterfaceManager::createDialog(Mantid::API::IAlgorithm* alg, QWidget* parent,
  bool forScript, const QHash<QString,QString> & preset_values, 
  const QString & optional_msg,  const QStringList & enabled, const QStringList & disabled)
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
  dlg->addEnabledAndDisableLists(enabled, disabled);

  // Setup the layout
  dlg->initializeLayout();

  return dlg;  
}

/**
 *  Create an algorithm dialog for a given algorithm name.
 * @param algorithmName : Name of the algorithm
 * @param forScript : True if this is being run from a script.
 * @param parent : Parent widget
 * @return new AlgorithmDialog
 */
AlgorithmDialog* InterfaceManager::createDialogFromName(const QString& algorithmName,  bool forScript, QWidget* parent)
{
    // Create the algorithm. This should throw if the algorithm can't be found.
    auto alg = Mantid::API::FrameworkManager::Instance().createAlgorithm(algorithmName.toStdString());

    // Forward call.
    return createDialog(alg, parent, forScript);
}

/**
 * Create a new instance of the correct type of UserSubWindow
 * @param interface_name :: The registered name of the interface
 * @param parent :: The parent widget
 */
UserSubWindow* InterfaceManager::createSubWindow(const QString & interface_name, QWidget* parent)
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
QStringList InterfaceManager::getUserSubWindowKeys() const
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
InterfaceManager::InterfaceManager()
{
  // Attempt to load libraries that may contain custom interface classes
  const std::string libpath = Mantid::Kernel::ConfigService::Instance().getString("mantidqt.plugins.directory");
  if( !libpath.empty() )
  {
    // Lazy loading. Avoid loading libraries every time a new instance is created.
    static bool isLoaded;
    if(!isLoaded)
    {
      int nloaded = Mantid::Kernel::LibraryManager::Instance().OpenAllLibraries(libpath);
      if( nloaded == 0 )
      {
        g_log.warning() << "Unable to load Qt plugin libraries.\n"
          << "Please check that the 'mantidqt.plugins.directory' variable in the .properties file points to "
          << "the correct location."
          << std::endl;
      }
      isLoaded = true;
    }
  }
}

/// Destructor
InterfaceManager::~InterfaceManager()
{
}

void InterfaceManager::registerVatesGuiFactory(Mantid::Kernel::AbstractInstantiator<VatesViewerInterface> *factory)
{
  m_vatesGuiFactory = factory;
}

/*
Getter to determine if vates components have been installed.
@return true if they are available.
*/
bool InterfaceManager::hasVatesLibraries()
{
  return NULL != m_vatesGuiFactory;
}



VatesViewerInterface *InterfaceManager::createVatesSimpleGui() const
{
  if(m_vatesGuiFactory == NULL)
  {
    g_log.error() << "InterfaceManager::createVatesSimpleGui is null. Mantid Vates package is probably not installed." << std::endl;
    throw Mantid::Kernel::Exception::NullPointerException("InterfaceManager::createVatesSimpleGui", "m_vatesGuiFactory");
  }
  else 
  {
    VatesViewerInterface *vsg = this->m_vatesGuiFactory->createUnwrappedInstance();
    if (!vsg)
    {
      g_log.error() << "Error creating Vates Simple GUI" << std::endl;
    }
    return vsg;
  }
}
