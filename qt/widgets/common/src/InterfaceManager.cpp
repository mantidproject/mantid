// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------
// Includes
//----------------------------------
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/AlgorithmDialogFactory.h"
#include "MantidQtWidgets/Common/GenericDialog.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"
#include "MantidQtWidgets/Common/PluginLibraries.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MantidQtWidgets/Common/UserSubWindowFactory.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <Poco/Environment.h>
#include <QStringList>
#include <QUrl>

using namespace MantidQt::API;
using Mantid::Kernel::AbstractInstantiator;

namespace {
// static logger
Mantid::Kernel::Logger g_log("InterfaceManager");

// Load libraries once
std::once_flag DLLS_LOADED;

// Track if message saying offline help is unavailable has been shown
// This might now refer to the new Python help system if it's not registered via the factory
bool helpSystemNotAvailableMsgDisplayed = false;

} // namespace

// initialise HelpWindow factory
Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *InterfaceManager::m_helpViewer = nullptr;

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
AlgorithmDialog *InterfaceManager::createDialog(const std::shared_ptr<Mantid::API::IAlgorithm> &alg, QWidget *parent,
                                                bool forScript, const QHash<QString, QString> &presetValues,
                                                const QString &optionalMsg, const QStringList &enabled,
                                                const QStringList &disabled) {
  AlgorithmDialog *dlg = nullptr;
  if (AlgorithmDialogFactory::Instance().exists(alg->name() + "Dialog")) {
    g_log.debug() << "Creating a specialised dialog for " << alg->name() << '\n';
    dlg = AlgorithmDialogFactory::Instance().createUnwrapped(alg->name() + "Dialog");
  } else {
    dlg = new GenericDialog;
    g_log.debug() << "No specialised dialog exists for the " << alg->name()
                  << " algorithm: a generic one has been created\n";
  }

  // The parent so that the dialog appears on top of it
  dlg->setParent(parent);
  dlg->setAttribute(Qt::WA_DeleteOnClose, true);

  // Set the QDialog window flags to ensure the dialog ends up on top
  Qt::WindowFlags flags = Qt::WindowFlags();
#ifdef Q_OS_MAC
  // Work around to ensure that floating windows remain on top of the main
  // application window, but below other applications on Mac
  // Note: Qt::Tool cannot have both a max and min button on OSX
  flags |= Qt::Tool;
  flags |= Qt::CustomizeWindowHint;
  flags |= Qt::WindowMinimizeButtonHint;
  flags |= Qt::WindowCloseButtonHint;
#else
  flags |= Qt::Dialog;
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
AlgorithmDialog *InterfaceManager::createDialogFromName(const QString &algorithmName, const int version,
                                                        QWidget *parent, bool forScript,
                                                        const QHash<QString, QString> &presetValues,
                                                        const QString &optionalMsg, const QStringList &enabled,
                                                        const QStringList &disabled) {
  // Create the algorithm. This should throw if the algorithm can't be found.
  auto alg = Mantid::API::AlgorithmManager::Instance().create(algorithmName.toStdString(), version);

  // Forward call.
  return createDialog(alg, parent, forScript, presetValues, optionalMsg, enabled, disabled);
}

/**
 * Create a new instance of the correct type of UserSubWindow
 * @param interface_name :: The registered name of the interface
 * @param parent :: The parent widget
 * @param isWindow :: Should the widget be an independent window
 */
UserSubWindow *InterfaceManager::createSubWindow(const QString &interface_name, QWidget *parent, bool isWindow) {
  UserSubWindow *user_win = nullptr;
  std::string iname = interface_name.toStdString();
  try {
    user_win = UserSubWindowFactory::Instance().createUnwrapped(iname);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    user_win = nullptr;
  }
  if (user_win) {
    g_log.debug() << "Created a specialised interface for " << iname << '\n';

    // set the parent. Note - setParent without flags parameter resets the flags
    // ie window becomes a child widget
    if (isWindow) {
      user_win->setParent(parent, user_win->windowFlags());
    } else {
      user_win->setParent(parent);
    }

    user_win->setInterfaceName(interface_name);
    user_win->initializeLayout();

    notifyExistingInterfaces(user_win);

  } else {
    g_log.error() << "Error creating interface " << iname << "\n";
  }
  return user_win;
}

/**
 * Notifies the existing interfaces that a new interface has been created, and
 * then notifies the new interface about the other interfaces which already
 * exist. This can be used to connect signals between interfaces (override
 * otherUserSubWindowCreated in the interface class).
 *
 * @param newWindow :: The interface just created
 */
void InterfaceManager::notifyExistingInterfaces(UserSubWindow *newWindow) {
  auto &existingWindows = existingInterfaces();

  for (auto &window : existingWindows)
    window->otherUserSubWindowCreated(newWindow);

  newWindow->otherUserSubWindowCreated(existingWindows);

  existingWindows.append(newWindow);
}

QList<QPointer<UserSubWindow>> &InterfaceManager::existingInterfaces() {
  static QList<QPointer<UserSubWindow>> existingSubWindows;
  existingSubWindows.erase(std::remove_if(existingSubWindows.begin(), existingSubWindows.end(),
                                          [](QPointer<UserSubWindow> &window) { return window.isNull(); }),
                           existingSubWindows.end());
  return existingSubWindows;
}

/**
 * The keys associated with UserSubWindow classes
 * @returns A QStringList containing the keys from the InterfaceFactory that
 * refer to UserSubWindow classes
 */
QStringList InterfaceManager::getUserSubWindowKeys() const { return UserSubWindowFactory::Instance().keys(); }

//----------------------------------
// Private member functions
//----------------------------------
/// Default Constructor
InterfaceManager::InterfaceManager() {
  // Attempt to load libraries that may contain custom interface classes
  std::call_once(DLLS_LOADED, []() { loadPluginsFromCfgPath("mantidqt.plugins.directory"); });
}

/// Destructor
InterfaceManager::~InterfaceManager() = default;

void InterfaceManager::registerHelpWindowFactory(Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *factory) {
  m_helpViewer = factory;
}

MantidHelpInterface *InterfaceManager::createHelpWindow() const {
  if (m_helpViewer == nullptr) {
    if (!helpSystemNotAvailableMsgDisplayed) {
      g_log.information("Help system not available (no viewer registered via factory).");
      helpSystemNotAvailableMsgDisplayed = true;
    }
    return nullptr;
  } else {
    MantidHelpInterface *interface = this->m_helpViewer->createUnwrappedInstance();
    if (!interface) {
      g_log.error("Error creating help window via factory registration.");
    }
    return interface;
  }
}

void InterfaceManager::showHelpPage(const QString &url) {
  auto window = createHelpWindow();
  if (window)
    window->showPage(url);
}

void InterfaceManager::showAlgorithmHelp(const QString &name, const int version) {
  auto window = createHelpWindow();
  if (window)
    window->showAlgorithm(name, version);
}

void InterfaceManager::showConceptHelp(const QString &name) {
  auto window = createHelpWindow();
  if (window)
    window->showConcept(name);
}

void InterfaceManager::showFitFunctionHelp(const QString &name) {
  auto window = createHelpWindow();
  if (window)
    window->showFitFunction(name);
}

void InterfaceManager::showCustomInterfaceHelp(const QString &name, const QString &area, const QString &section) {
  auto window = createHelpWindow();
  if (window) {
    QString pagePath = QString("interfaces/%1/%2.html").arg(area).arg(name);
    if (!section.isEmpty()) {
      pagePath += QString("#%1").arg(section);
    }
    window->showPage(pagePath);
  }
}

void InterfaceManager::showWebPage(const QString &url) { MantidDesktopServices::openUrl(QUrl(url)); }

void InterfaceManager::closeHelpWindow() {
  g_log.debug("InterfaceManager::closeHelpWindow() called. Note: Direct management of a single help window instance is "
              "deprecated.");
}

void InterfaceManager::showHomeHelpPage() {
  auto window = createHelpWindow();
  if (window)
    window->showPage(QString());
}

void InterfaceManager::cleanup() { g_log.debug("InterfaceManager::cleanup() called."); }
