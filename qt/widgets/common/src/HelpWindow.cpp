// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"

#include <QUrl>
#include <QWidget>
#include <boost/lexical_cast.hpp>

namespace MantidQt {
namespace API {
namespace {
/// static logger
Mantid::Kernel::Logger g_log("HelpWindow");

/**
 * Attach the parent to the gui and connect the shutdown signal
 *
 * @param gui The help window that will render the url.
 * @param parent The parent widget.
 */
void connectParent(MantidHelpInterface *gui, QWidget *parent) {
  if (parent) {
    if (parent->metaObject()->indexOfSignal("shutting_down") > 0) {
      QObject::connect(parent, SIGNAL(shutting_down()), gui, SLOT(shutdown()));
    }
    gui->setParent(parent);
  }
}
} // namespace

using std::string;

void HelpWindow::showPage(QWidget *parent, const std::string &url) {
  showPage(parent, QString(url.c_str()));
}

void HelpWindow::showPage(QWidget *parent, const QString &url) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    connectParent(gui, parent);
    gui->showPage(url);
  } else {
    g_log.error() << "Failed to launch help for page " << url.toStdString()
                  << "\n";
  }
}

void HelpWindow::showPage(QWidget *parent, const QUrl &url) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    connectParent(gui, parent);
    gui->showPage(url);
  } else {
    g_log.error() << "Failed to launch help for page "
                  << url.toString().toStdString() << "\n";
  }
}

void HelpWindow::showAlgorithm(QWidget *parent, const std::string &name,
                               const int version) {
  showAlgorithm(parent, QString(name.c_str()), version);
}

void HelpWindow::showAlgorithm(QWidget *parent, const QString &name,
                               const int version) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    connectParent(gui, parent);
    gui->showAlgorithm(name, version);
  } else {
    g_log.error() << "Failed to launch help for algorithm "
                  << name.toStdString() << " v" << version << "\n";
  }
}

void HelpWindow::showConcept(QWidget *parent, const std::string &name) {
  showConcept(parent, QString(name.c_str()));
}

void HelpWindow::showConcept(QWidget *parent, const QString &name) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    connectParent(gui, parent);
    gui->showConcept(name);
  } else {
    g_log.error() << "Failed to launch help for concept " << name.toStdString()
                  << "\n";
  }
}

void HelpWindow::showFitFunction(QWidget *parent, const std::string &name) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    connectParent(gui, parent);
    gui->showFitFunction(name);
  } else {
    g_log.error() << "Failed to launch help for fit function " << name << "\n";
  }
}

void HelpWindow::showCustomInterface(QWidget *parent, const std::string &name,
                                     const std::string &section) {
  showCustomInterface(parent, QString::fromStdString(name),
                      QString::fromStdString(section));
}

void HelpWindow::showCustomInterface(QWidget *parent, const QString &name,
                                     const QString &section) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    connectParent(gui, parent);
    gui->showCustomInterface(name, section);
  } else {
    g_log.error() << "Failed to launch help for custom interface "
                  << name.toStdString() << "\n";
  }
}

} // namespace API
} // namespace MantidQt
