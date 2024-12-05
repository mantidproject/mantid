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
#include "MantidQtWidgets/Common/MantidHelpWindow.h"
#include "MantidQtWidgets/Common/PluginLibraries.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MantidQtWidgets/Common/UserSubWindowFactory.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <Poco/Environment.h>
#include <QStringList>
#include <boost/python.hpp>

using namespace MantidQt::API;
using Mantid::Kernel::AbstractInstantiator;
using MantidQt::MantidWidgets::MantidHelpWindow;

namespace {
// static logger
Mantid::Kernel::Logger g_log("InterfaceManager");

// Load libraries once
std::once_flag DLLS_LOADED;

// Track if message saying offline help is unavailable has been shown
bool offlineHelpMsgDisplayed = false;

} // namespace

// Initialise HelpWindow factory
Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *InterfaceManager::m_helpViewer = nullptr;

//----------------------------------
// Public member functions
//----------------------------------

/**
 * Show a help page using the default or Python-based HelpWindow
 * @param url :: URL of the page to be displayed
 */
void InterfaceManager::showHelpPage(const QString &url) {
  auto window = createHelpWindow();
  if (window) {
    window->showPage(url);
  } else {
    launchPythonHelp(url);
  }
}

/**
 * Launch the Python-based HelpWindow
 * @param url :: URL to pass to the Python HelpWindow
 */
void InterfaceManager::launchPythonHelp(const QString &url) {
  try {
    boost::python::object mainNamespace = boost::python::import("__main__").attr("__dict__");
    boost::python::exec("from helpwindowpresenter import HelpPresenter", mainNamespace);

    std::string currentPage = url.toStdString();
    boost::python::exec(("presenter = HelpPresenter.instance(); presenter.show_page('" + currentPage + "')").c_str(),
                        mainNamespace);

    g_log.information("Launched Python-based HelpWindow.");
  } catch (const boost::python::error_already_set &) {
    PyErr_Print();
    g_log.error("Failed to launch Python HelpWindow.");
  }
}

void InterfaceManager::showAlgorithmHelp(const QString &name, const int version) {
  auto window = createHelpWindow();
  if (window) {
    window->showAlgorithm(name, version);
  } else {
    QString url = QString("file:///path/to/docs/algorithms/%1-v%2.html").arg(name).arg(version);
    launchPythonHelp(url);
  }
}

void InterfaceManager::showConceptHelp(const QString &name) {
  auto window = createHelpWindow();
  if (window) {
    window->showConcept(name);
  } else {
    QString url = QString("file:///path/to/docs/concepts/%1.html").arg(name);
    launchPythonHelp(url);
  }
}

void InterfaceManager::showFitFunctionHelp(const QString &name) {
  auto window = createHelpWindow();
  if (window) {
    window->showFitFunction(name);
  } else {
    g_log.error("Fit function help is not implemented in the Python-based HelpWindow.");
  }
}

void InterfaceManager::showCustomInterfaceHelp(const QString &name, const QString &area, const QString &section) {
  auto window = createHelpWindow();
  if (window) {
    window->showCustomInterface(name, area, section);
  } else {
    QString url = QString("file:///path/to/docs/interfaces/%1/%2.html").arg(area).arg(name);
    launchPythonHelp(url);
  }
}

void InterfaceManager::showWebPage(const QString &url) { MantidDesktopServices::openUrl(url); }

/**
 * Closes the active HelpWindow, if any.
 */
void InterfaceManager::closeHelpWindow() {
  if (MantidHelpWindow::helpWindowExists()) {
    auto window = createHelpWindow();
    window->shutdown();
  }
}

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

/**
 * Register the HelpWindow factory
 * @param factory :: The factory instance
 */
void InterfaceManager::registerHelpWindowFactory(Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *factory) {
  m_helpViewer = factory;
}

/**
 * Create an instance of the appropriate HelpWindow
 */
MantidHelpInterface *InterfaceManager::createHelpWindow() const {
  if (m_helpViewer == nullptr) {
    if (!offlineHelpMsgDisplayed) {
      g_log.information("Offline help is not available in this version of Workbench.");
      offlineHelpMsgDisplayed = true;
    }
    return nullptr;
  } else {
    auto *interface = m_helpViewer->createUnwrappedInstance();
    if (!interface) {
      g_log.error("Error creating help window");
    }
    return interface;
  }
}
