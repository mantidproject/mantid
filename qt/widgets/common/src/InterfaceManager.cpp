// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------
// Includes
//----------------------------------
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/GenericDialog.h"
#include "MantidQtWidgets/Common/InterfaceFactory.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"
#include "MantidQtWidgets/Common/PluginLibraries.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MantidQtWidgets/Common/VatesViewerInterface.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <Poco/Environment.h>
#include <QStringList>

using namespace MantidQt::API;
using Mantid::Kernel::AbstractInstantiator;

namespace {
// static logger
Mantid::Kernel::Logger g_log("InterfaceManager");

// Load libraries once
std::once_flag DLLS_LOADED;

// Track if message saying offline help is unavailable has been shown
bool offlineHelpMsgDisplayed = false;
} // namespace

// initialise VATES factory
Mantid::Kernel::AbstractInstantiator<VatesViewerInterface>
    *InterfaceManager::m_vatesGuiFactory = nullptr;
// initialise HelpWindow factory
Mantid::Kernel::AbstractInstantiator<MantidHelpInterface>
    *InterfaceManager::m_helpViewer = nullptr;

//----------------------------------
// Public member functions
//----------------------------------
/**
 * Return a specialized dialog for the given algorithm. If none exists then the
 * default is returned
 * @param alg :: A pointer to the algorithm
 * @param parent :: An optional parent widget
 * @param forScript :: A boolean indicating if this dialog is to be use for from
 * a script or not. If true disables the autoexecution of the dialog
 * @param presetValues :: A hash of property names to preset values for the
 * dialog
 * @param optionalMsg :: An optional message string to be placed at the top of
 * the dialog
 * @param enabled :: These properties will be left enabled
 * @param disabled :: These properties will be left disabled
 * @returns An AlgorithmDialog object
 */
AlgorithmDialog *InterfaceManager::createDialog(
    boost::shared_ptr<Mantid::API::IAlgorithm> alg, QWidget *parent,
    bool forScript, const QHash<QString, QString> &presetValues,
    const QString &optionalMsg, const QStringList &enabled,
    const QStringList &disabled) {
  AlgorithmDialog *dlg = nullptr;
  if (AlgorithmDialogFactory::Instance().exists(alg->name() + "Dialog")) {
    g_log.debug() << "Creating a specialised dialog for " << alg->name()
                  << '\n';
    dlg = AlgorithmDialogFactory::Instance().createUnwrapped(alg->name() +
                                                             "Dialog");
  } else {
    dlg = new GenericDialog;
    g_log.debug() << "No specialised dialog exists for the " << alg->name()
                  << " algorithm: a generic one has been created\n";
  }

  // The parent so that the dialog appears on top of it
  dlg->setParent(parent);
  dlg->setAttribute(Qt::WA_DeleteOnClose, true);

  // Set the QDialog window flags to ensure the dialog ends up on top
  Qt::WindowFlags flags = nullptr;
  flags |= Qt::Dialog;
  flags |= Qt::WindowCloseButtonHint;
#ifdef Q_OS_MAC
  // Work around to ensure that floating windows remain on top of the main
  // application window, but below other applications on Mac
  // Note: Qt::Tool cannot have both a max and min button on OSX
  flags = 0; // NOLINT
  flags |= Qt::Tool;
  flags |= Qt::CustomizeWindowHint;
  flags |= Qt::WindowMinimizeButtonHint;
  flags |= Qt::WindowCloseButtonHint;
#endif
  dlg->setWindowFlags(flags);

  // Set the content
  dlg->setAlgorithm(alg);
  dlg->setPresetValues(presetValues);
  dlg->isForScript(forScript);
  dlg->setOptionalMessage(optionalMsg);
  dlg->addEnabledAndDisableLists(enabled, disabled);

  // Setup the layout
  dlg->initializeLayout();

  if (forScript)
    dlg->executeOnAccept(false); // override default
  return dlg;
}

/**
 * @param algorithmName :: Name of AlgorithmDialog
 * @param version :: Version number
 * @param parent :: An optional parent widget
 * @param forScript :: A boolean indicating if this dialog is to be use for from
 * a script or not
 * @param presetValues :: A hash of property names to preset values for the
 * dialog
 * @param optionalMsg :: An optional message string to be placed at the top of
 * the dialog
 * @param enabled :: These properties will be left enabled
 * @param disabled :: These properties will be left disabled
 */
AlgorithmDialog *InterfaceManager::createDialogFromName(
    const QString &algorithmName, const int version, QWidget *parent,
    bool forScript, const QHash<QString, QString> &presetValues,
    const QString &optionalMsg, const QStringList &enabled,
    const QStringList &disabled) {
  // Create the algorithm. This should throw if the algorithm can't be found.
  auto alg = Mantid::API::AlgorithmManager::Instance().create(
      algorithmName.toStdString(), version);

  // Forward call.
  return createDialog(alg, parent, forScript, presetValues, optionalMsg,
                      enabled, disabled);
}

/**
 * Create a new instance of the correct type of UserSubWindow
 * @param interface_name :: The registered name of the interface
 * @param parent :: The parent widget
 */
UserSubWindow *InterfaceManager::createSubWindow(const QString &interface_name,
                                                 QWidget *parent) {
  UserSubWindow *user_win = nullptr;
  std::string iname = interface_name.toStdString();
  try {
    user_win = UserSubWindowFactory::Instance().createUnwrapped(iname);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    user_win = nullptr;
  }
  if (user_win) {
    g_log.debug() << "Created a specialised interface for " << iname << '\n';
    user_win->setParent(parent);
    user_win->setInterfaceName(interface_name);
    user_win->initializeLayout();
  } else {
    g_log.error() << "Error creating interface " << iname << "\n";
  }
  return user_win;
}

/**
 * The keys associated with UserSubWindow classes
 * @returns A QStringList containing the keys from the InterfaceFactory that
 * refer to UserSubWindow classes
 */
QStringList InterfaceManager::getUserSubWindowKeys() const {
  QStringList key_list;
  std::vector<std::string> keys = UserSubWindowFactory::Instance().getKeys();
  std::vector<std::string>::const_iterator iend = keys.end();
  for (std::vector<std::string>::const_iterator itr = keys.begin(); itr != iend;
       ++itr) {
    key_list.append(QString::fromStdString(*itr));
  }
  return key_list;
}

//----------------------------------
// Private member functions
//----------------------------------
/// Default Constructor
InterfaceManager::InterfaceManager() {
  // Attempt to load libraries that may contain custom interface classes
  std::call_once(DLLS_LOADED, []() {
    loadPluginsFromCfgPath("mantidqt.plugins.directory");
  });
}

/// Destructor
InterfaceManager::~InterfaceManager() {}

void InterfaceManager::registerVatesGuiFactory(
    Mantid::Kernel::AbstractInstantiator<VatesViewerInterface> *factory) {
  m_vatesGuiFactory = factory;
}

/*
Getter to determine if vates components have been installed.
@return true if they are available.
*/
bool InterfaceManager::hasVatesLibraries() {
  return nullptr != m_vatesGuiFactory;
}

VatesViewerInterface *InterfaceManager::createVatesSimpleGui() const {
  if (m_vatesGuiFactory == nullptr) {
    g_log.error() << "InterfaceManager::createVatesSimpleGui is null. Mantid "
                     "Vates package is probably not installed.\n";
    throw Mantid::Kernel::Exception::NullPointerException(
        "InterfaceManager::createVatesSimpleGui", "m_vatesGuiFactory");
  } else {
    VatesViewerInterface *vsg =
        this->m_vatesGuiFactory->createUnwrappedInstance();
    if (!vsg) {
      g_log.error() << "Error creating Vates Simple GUI\n";
    }
    return vsg;
  }
}

void InterfaceManager::registerHelpWindowFactory(
    Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *factory) {
  m_helpViewer = factory;
}

MantidHelpInterface *InterfaceManager::createHelpWindow() const {
  if (m_helpViewer == nullptr) {
    if (!offlineHelpMsgDisplayed) {
      g_log.information(
          "Offline help is not available in this version of Workbench.");
      offlineHelpMsgDisplayed = true;
    }
    return nullptr;
  } else {
    MantidHelpInterface *interface =
        this->m_helpViewer->createUnwrappedInstance();
    if (!interface) {
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

void InterfaceManager::showWebPage(const QString &url) {
  MantidDesktopServices::openUrl(url);
}
