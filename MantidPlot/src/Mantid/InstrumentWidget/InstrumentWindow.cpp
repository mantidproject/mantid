// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "InstrumentWindow.h"
#include "ApplicationWindow.h"
#include "Mantid/MantidUI.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include <QApplication>
#include <QMessageBox>

#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"

// Register the window into the WindowFactory
DECLARE_WINDOW(InstrumentWindow)

using namespace Mantid;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

InstrumentWindow::InstrumentWindow(const QString &wsName, const QString &label,
                                   ApplicationWindow *parent,
                                   const QString &name)
    : MdiSubWindow(parent, label, name, nullptr) {

  m_instrumentWidget = new InstrumentWidget(wsName, this);
  this->setWidget(m_instrumentWidget);
  confirmClose(parent->confirmCloseInstrWindow);
  resize(m_instrumentWidget->size());

  connect(m_instrumentWidget, SIGNAL(preDeletingHandle()), this,
          SLOT(closeSafely()));
  connect(m_instrumentWidget, SIGNAL(clearingHandle()), this,
          SLOT(closeSafely()));
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Interface", "InstrumentView", false);
}

InstrumentWindow::~InstrumentWindow() {}

/**
 * Load instrument window state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 * @param app :: handle to the application window
 * @param fileVersion :: version of the Mantid project file
 * @return handle to the created instrument window
 */
MantidQt::API::IProjectSerialisable *InstrumentWindow::loadFromProject(
    const std::string &lines, ApplicationWindow *app, const int fileVersion) {
  Q_UNUSED(fileVersion);

  MantidQt::API::TSVSerialiser tsv(lines);

  if (!tsv.selectLine("WorkspaceName"))
    return nullptr;

  const auto name = tsv.asQString(1);
  const auto workspace = app->mantidUI->getWorkspace(name);
  const auto ws = boost::dynamic_pointer_cast<const MatrixWorkspace>(workspace);

  if (!ws)
    return nullptr;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  auto instr = ws->getInstrument();
  if (!instr || instr->getName().empty()) {
    QApplication::restoreOverrideCursor();
    QMessageBox::critical(app, "MantidPlot - Error",
                          "Instrument view cannot be opened");
    return nullptr;
  }

  // Create a new window
  const QString windowName("InstrumentWindow:" + name);
  auto iw = new InstrumentWindow(name, "Instrument", app, windowName);

  // Populate window properties
  try {
    if (tsv.hasLine("geometry")) {
      const auto geometry = tsv.lineAsQString("geometry");
      app->restoreWindowGeometry(app, iw, geometry);
    }

    iw->m_instrumentWidget->loadFromProject(lines);
    app->addMdiSubWindow(iw);

    QApplication::restoreOverrideCursor();
    return iw;
  } catch (const std::exception &e) {
    QApplication::restoreOverrideCursor();
    QString errorMessage =
        "Instrument view cannot be created:\n\n" + QString(e.what());
    QMessageBox::critical(app, "MantidPlot - Error", errorMessage);
  }

  return nullptr;
}

std::vector<std::string> InstrumentWindow::getWorkspaceNames() {
  return {m_instrumentWidget->getWorkspaceNameStdString()};
}

std::string InstrumentWindow::getWindowName() {
  return m_instrumentWidget->windowTitle().toStdString();
}

/**
 * Save the state of the instrument window to a Mantid project file
 * @param app :: handle to the current application window instance
 * @return a string representing the state of the instrument window
 */
std::string InstrumentWindow::saveToProject(ApplicationWindow *app) {
  MantidQt::API::TSVSerialiser tsv, window;
  window.writeRaw(app->windowGeometryInfo(this));
  auto widgetContents = m_instrumentWidget->saveToProject();
  window.writeRaw(widgetContents);
  tsv.writeSection("instrumentwindow", window.outputLines());
  return tsv.outputLines();
}

void InstrumentWindow::selectTab(int tab) {
  return m_instrumentWidget->selectTab(tab);
}

InstrumentWidgetTab *InstrumentWindow::getTab(const QString &title) const {
  return m_instrumentWidget->getTab(title);
}

InstrumentWidgetTab *InstrumentWindow::getTab(int tab) const {
  return m_instrumentWidget->getTab((InstrumentWidget::Tab)tab);
}

void InstrumentWindow::setBinRange(double min_value, double max_value) {
  return m_instrumentWidget->setBinRange(min_value, max_value);
}

bool InstrumentWindow::overlay(const QString &wsName) {
  return m_instrumentWidget->overlay(wsName);
}

void InstrumentWindow::changeColormap() {
  return m_instrumentWidget->changeColormap();
}

void InstrumentWindow::changeColormap(const QString &file) {
  return m_instrumentWidget->changeColormap(file);
}

void InstrumentWindow::setColorMapMinValue(double min_value) {
  return m_instrumentWidget->setColorMapMinValue(min_value);
}

void InstrumentWindow::setColorMapMaxValue(double max_value) {
  return m_instrumentWidget->setColorMapMaxValue(max_value);
}

void InstrumentWindow::setColorMapRange(double min_value, double max_value) {
  return m_instrumentWidget->setColorMapRange(min_value, max_value);
}

void InstrumentWindow::selectComponent(const QString &name) {
  return m_instrumentWidget->selectComponent(name);
}

void InstrumentWindow::setScaleType(GraphOptions::ScaleType type) {
  return m_instrumentWidget->setScaleType(
      static_cast<MantidColorMap::ScaleType>(type));
}

void InstrumentWindow::setViewType(const QString &type) {
  return m_instrumentWidget->setViewType(type);
}

void InstrumentWindow::closeSafely() {
  confirmClose(false);
  close();
}
