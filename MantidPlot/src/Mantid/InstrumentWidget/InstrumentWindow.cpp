#include "ApplicationWindow.h"
#include "Mantid/MantidUI.h"
#include "InstrumentWindow.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/UsageService.h"
#include "TSVSerialiser.h"

#include <QApplication>
#include <QMessageBox>

#include <MantidQtMantidWidgets/InstrumentView/InstrumentWidget.h>
#include <MantidQtMantidWidgets/InstrumentView/ProjectionSurface.h>

// Register the window into the WindowFactory
DECLARE_WINDOW(InstrumentWindow)

using namespace Mantid;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

InstrumentWindow::InstrumentWindow(const QString &wsName, const QString &label,
                                   ApplicationWindow *parent,
                                   const QString &name)
    : MdiSubWindow(parent, label, name, 0) {

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

IProjectSerialisable *InstrumentWindow::loadFromProject(
    const std::string &lines, ApplicationWindow *app, const int fileVersion) {
  Q_UNUSED(fileVersion);

  TSVSerialiser tsv(lines);
  if (tsv.selectLine("WorkspaceName")) {
    std::string wsName = tsv.asString(1);
    QString name = QString::fromStdString(wsName);

    if (!Mantid::API::AnalysisDataService::Instance().doesExist(wsName))
      return nullptr;
    MatrixWorkspace_const_sptr ws =
        boost::dynamic_pointer_cast<const MatrixWorkspace>(
            app->mantidUI->getWorkspace(QString::fromStdString(wsName)));
    if (!ws)
      return nullptr;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    Mantid::Geometry::Instrument_const_sptr instr = ws->getInstrument();
    if (!instr || instr->getName().empty()) {
      QApplication::restoreOverrideCursor();
      QMessageBox::critical(app, "MantidPlot - Error",
                            "Instrument view cannot be opened");
      return nullptr;
    }

    // Need a new window
    const QString windowName(QString("InstrumentWindow:") +
                             QString::fromStdString(wsName));
    auto iw =
        new InstrumentWindow(name, QString("Instrument"), app, windowName);

    try {
      iw->selectTab(-1);

      if (tsv.hasLine("geometry")) {
        const QString geometry =
            QString::fromStdString(tsv.lineAsString("geometry"));
        app->restoreWindowGeometry(app, iw, geometry);
      }

      app->addMdiSubWindow(iw);

      QApplication::restoreOverrideCursor();
      return iw;
    } catch (const std::exception &e) {
      QApplication::restoreOverrideCursor();
      QString errorMessage =
          "Instrument view cannot be created:\n\n" + QString(e.what());
      QMessageBox::critical(app, "MantidPlot - Error", errorMessage);
    }
  }

  return nullptr;
}

std::string InstrumentWindow::saveToProject(ApplicationWindow *app) {
  TSVSerialiser tsv;
  tsv.writeRaw("<instrumentwindow>");
  tsv.writeLine("WorkspaceName")
      << m_instrumentWidget->getWorkspaceNameStdString();
  tsv.writeRaw(app->windowGeometryInfo(this));
  tsv.writeRaw("</instrumentwindow>");
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
  return m_instrumentWidget->setScaleType(type);
}

void InstrumentWindow::setViewType(const QString &type) {
  return m_instrumentWidget->setViewType(type);
}

void InstrumentWindow::closeSafely() {
  confirmClose(false);
  close();
}
