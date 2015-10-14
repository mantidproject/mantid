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
#include "MantidQtAPI/MantidHelpInterface.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/Exception.h"

#include <QStringList>

using namespace MantidQt::API;

namespace
{
  // static logger
  Mantid::Kernel::Logger g_log("InterfaceManager");
}

// initialise VATES factory
Mantid::Kernel::AbstractInstantiator<VatesViewerInterface> *InterfaceManager::m_vatesGuiFactory = NULL;
// initialise HelpWindow factory
Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *InterfaceManager::m_helpViewer = NULL;

//----------------------------------
// Public member functions
//----------------------------------
/**
 * Return a specialized dialog for the given algorithm. If none exists then the default is returned
 * @param alg :: A pointer to the algorithm
 * @param parent :: An optional parent widget
 * @param forScript :: A boolean indicating if this dialog is to be use for from a script or not. If true disables the autoexecution of the dialog
 * @param presetValues :: A hash of property names to preset values for the dialog
 * @param optionalMsg :: An optional message string to be placed at the top of the dialog
 * @param enabled :: These properties will be left enabled
 * @param disabled :: These properties will be left disabled
 * @returns An AlgorithmDialog object
 */
AlgorithmDialog*
InterfaceManager::createDialog(boost::shared_ptr<Mantid::API::IAlgorithm> alg, QWidget* parent,
                               bool forScript, const QHash<QString,QString> & presetValues,
                               const QString & optionalMsg,  const QStringList & enabled, const QStringList & disabled)
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
  dlg->setAttribute(Qt::WA_DeleteOnClose, true);
  
  // Set the QDialog window flags to ensure the dialog ends up on top
  Qt::WindowFlags flags = 0;
  flags |= Qt::Dialog;
  flags |= Qt::WindowContextHelpButtonHint;
  dlg->setWindowFlags(flags);

  // Set the content
  dlg->setAlgorithm(alg);
  dlg->setPresetValues(presetValues);
  dlg->isForScript(forScript);
  dlg->setOptionalMessage(optionalMsg);
  dlg->addEnabledAndDisableLists(enabled, disabled);

  // Setup the layout
  dlg->initializeLayout();

  if(forScript) dlg->executeOnAccept(false); //override default
  return dlg;
}

/**
 * @param algorithmName :: Name of AlgorithmDialog
 * @param version :: Version number
 * @param parent :: An optional parent widget
 * @param forScript :: A boolean indicating if this dialog is to be use for from a script or not
 * @param presetValues :: A hash of property names to preset values for the dialog
 * @param optionalMsg :: An optional message string to be placed at the top of the dialog
 * @param enabled :: These properties will be left enabled
 * @param disabled :: These properties will be left disabled
 */
AlgorithmDialog* InterfaceManager::createDialogFromName(const QString& algorithmName, const int version, QWidget* parent, bool forScript,
                                                        const QHash<QString, QString> &presetValues,
                                                        const QString &optionalMsg, const QStringList &enabled, const QStringList &disabled)
{
    // Create the algorithm. This should throw if the algorithm can't be found.
    auto alg = Mantid::API::AlgorithmManager::Instance().create(algorithmName.toStdString(), version);

    // Forward call.
    return createDialog(alg, parent, forScript, presetValues, optionalMsg, enabled, disabled);
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


void InterfaceManager::registerHelpWindowFactory(Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *factory)
{
  m_helpViewer = factory;
}

MantidHelpInterface *InterfaceManager::createHelpWindow() const
{
  if(m_helpViewer == NULL)
  {
    g_log.error("InterfaceManager::createHelpWindow is null.");
    throw Mantid::Kernel::Exception::NullPointerException("InterfaceManager::createHelpWindow", "m_helpViewer");
  }
  else
  {
    MantidHelpInterface *interface = this->m_helpViewer->createUnwrappedInstance();
    if (!interface)
    {
      g_log.error("Error creating help window");
    }
    return interface;
  }
}

void InterfaceManager::showHelpPage(const QString &url) {
  auto window = createHelpWindow();
  window->showPage(url);
}

void InterfaceManager::showWikiPage(const QString &page) {
  auto window = createHelpWindow();
  window->showWikiPage(page);
}

void InterfaceManager::showAlgorithmHelp(const QString &name,
                                         const int version) {
  auto window = createHelpWindow();
  window->showAlgorithm(name, version);
}

void InterfaceManager::showConceptHelp(const QString &name) {
  auto window = createHelpWindow();
  window->showConcept(name);
}

void InterfaceManager::showFitFunctionHelp(const QString &name) {
  auto window = createHelpWindow();
  window->showFitFunction(name);
}

void InterfaceManager::showCustomInterfaceHelp(const QString &name) {
  auto window = createHelpWindow();
  window->showCustomInterface(name);
}
