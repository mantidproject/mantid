// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SpectrumViewer/SpectrumView.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#include "MantidQtWidgets/SpectrumViewer/ColorMaps.h"
#include "MantidQtWidgets/SpectrumViewer/EModeHandler.h"
#include "MantidQtWidgets/SpectrumViewer/MatrixWSDataSource.h"
#include "MantidQtWidgets/SpectrumViewer/RangeHandler.h"
#include "MantidQtWidgets/SpectrumViewer/SVConnections.h"
#include "MantidQtWidgets/SpectrumViewer/SliderHandler.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDisplay.h"

#include <QDropEvent>
#include <QSettings>
#include <boost/make_shared.hpp>

namespace MantidQt {
namespace SpectrumView {

/**
 *  Construct an SpectrumView to display data from the specified data source.
 *  The specified SpectrumDataSource must be constructed elsewhere and passed
 *  into this SpectrumView constructor.  Most other components of the
 *SpectrumView
 *  are managed by this class.  That is the graphs, image display and other
 *  parts of the SpectrumView are constructed here and are deleted when the
 *  SpectrumView destructor is called.
 *
 *  @param parent Top-level widget for object.
 */
SpectrumView::SpectrumView(QWidget *parent)
    : QMainWindow(parent), WorkspaceObserver(), m_ui(new Ui::SpectrumViewer()),
      m_sliderHandler(nullptr), m_rangeHandler(nullptr),
      m_emodeHandler(nullptr) {
  m_ui->setupUi(this);
  connect(m_ui->imageTabs, SIGNAL(currentChanged(int)), this,
          SLOT(changeSpectrumDisplay(int)));
  connect(m_ui->imageTabs, SIGNAL(tabCloseRequested(int)), this,
          SLOT(respondToTabCloseReqest(int)));
  connect(m_ui->tracking_always_on, SIGNAL(toggled(bool)), this,
          SLOT(changeTracking(bool)));
  updateHandlers();
  setAcceptDrops(true);
  loadSettings();

  // Watch for the deletion of the associated workspace
  observeAfterReplace();
  observePreDelete();
  observeADSClear();
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Interface", "SpectrumView", false);

#ifdef Q_OS_MAC
  // Work around to ensure that floating windows remain on top of the main
  // application window, but below other applications on Mac
  // Note: Qt::Tool cannot have both a max and min button on OSX
  auto flags = windowFlags();
  flags |= Qt::Tool;
  flags |= Qt::CustomizeWindowHint;
  flags |= Qt::WindowMinimizeButtonHint;
  flags |= Qt::WindowCloseButtonHint;
  setWindowFlags(flags);
#endif
}

SpectrumView::~SpectrumView() {
  saveSettings();
  if (m_emodeHandler)
    delete m_emodeHandler;
}

/**
 * Handles the resize event fo rthe window.
 *
 * Used to keep the image splitters in the correct position and
 * in alighment with each other.
 *
 * @param event The resize event
 */
void SpectrumView::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  if (m_svConnections) {
    m_svConnections->imageSplitterMoved();
  }
}

/**
 * Renders a new workspace on the spectrum viewer. If there is a workspace being
 *tracked, update that data source.
 *
 * @param wksp The matrix workspace to render
 */
void SpectrumView::renderWorkspace(
    Mantid::API::MatrixWorkspace_const_sptr wksp) {

  // Handle rendering of a workspace we already track
  if (replaceExistingWorkspace(wksp->getName(), wksp))
    return;

  auto dataSource = MatrixWSDataSource_sptr(new MatrixWSDataSource(wksp));
  m_dataSource.push_back(dataSource);

  // If we have a MatrixWSDataSource give it the handler for the
  // EMode, so the user can set EMode and EFixed.  NOTE: we could avoid
  // this type checking if we made the ui in the calling code and passed
  // it in.  We would need a common base class for this class and
  // the ref-viewer UI.
  dataSource->setEModeHandler(m_emodeHandler);

  // Connect WorkspaceObserver signals
  connect(this, SIGNAL(needToClose()), this, SLOT(closeWindow()));

  // Set the window title
  std::string windowTitle = "SpectrumView (" + wksp->getTitle() + ")";
  this->setWindowTitle(QString::fromStdString(windowTitle).simplified());

  auto spectrumPlot = m_ui->spectrumPlot;
  bool isFirstPlot = m_spectrumDisplay.isEmpty();

  int tab = 0;
  if (isFirstPlot) {
    m_ui->imageTabs->setTabText(
        m_ui->imageTabs->indexOf(m_ui->imageTabs->currentWidget()),
        QString::fromStdString(wksp->getName()));
    m_hGraph = boost::make_shared<GraphDisplay>(m_ui->h_graphPlot,
                                                m_ui->h_graph_table, false);
    m_vGraph = boost::make_shared<GraphDisplay>(m_ui->v_graphPlot,
                                                m_ui->v_graph_table, true);
  } else {
    spectrumPlot = new QwtPlot(this);
    auto widget = new QWidget();
    auto layout = new QHBoxLayout();
    layout->addWidget(spectrumPlot);
    widget->setLayout(layout);
    tab = m_ui->imageTabs->addTab(widget,
                                  QString::fromStdString(wksp->getName()));
    m_ui->imageTabs->setTabsClosable(true);
  }

  auto spectrumDisplay = boost::make_shared<SpectrumDisplay>(
      spectrumPlot, m_sliderHandler, m_rangeHandler, m_hGraph.get(),
      m_vGraph.get(), m_ui->image_table, isTrackingOn());
  spectrumDisplay->setDataSource(dataSource);

  if (isFirstPlot) {
    m_svConnections = boost::make_shared<SVConnections>(
        m_ui, this, spectrumDisplay.get(), m_hGraph.get(), m_vGraph.get());
    connect(this, SIGNAL(spectrumDisplayChanged(SpectrumDisplay *)),
            m_svConnections.get(), SLOT(setSpectrumDisplay(SpectrumDisplay *)));
    m_svConnections->imageSplitterMoved();
  } else {
    foreach (boost::shared_ptr<SpectrumDisplay> sd, m_spectrumDisplay) {
      sd->addOther(spectrumDisplay);
    }
    spectrumDisplay->addOthers(m_spectrumDisplay);
  }

  m_spectrumDisplay.append(spectrumDisplay);
  m_ui->imageTabs->setCurrentIndex(tab);
}

/**
 * Renders a new workspace on the spectrum viewer.
 *
 * @param wsName The name of the matrix workspace to render
 */
void SpectrumView::renderWorkspace(const QString &wsName) {
  Mantid::API::MatrixWorkspace_const_sptr wksp =
      Mantid::API::AnalysisDataService::Instance()
          .retrieveWS<const Mantid::API::MatrixWorkspace>(wsName.toStdString());

  renderWorkspace(wksp);
}

/**
 * Setup the various handlers (energy-mode, slider, range) for UI controls.
 */
void SpectrumView::updateHandlers() {
  m_emodeHandler = new EModeHandler(m_ui);
  m_sliderHandler = new SliderHandler(m_ui);
  m_rangeHandler = new RangeHandler(m_ui);
}

/**
 * Slot to close the window.
 */
void SpectrumView::closeWindow() { close(); }

/**
 * Signal to close this window if the workspace has just been deleted.
 *
 * @param wsName Name of workspace
 * @param ws Pointer to workspace
 */
void SpectrumView::preDeleteHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  if (m_spectrumDisplay.front()->hasData(wsName, ws)) {
    emit needToClose();
  }
}

/**
 * Signal that the workspace being looked at was just replaced with a different
 *one.
 *
 * @param wsName Name of workspace
 * @param ws Pointer to workspace
 */
void SpectrumView::afterReplaceHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  // We would only ever be replacing a workspace here
  replaceExistingWorkspace(
      wsName, boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws));
}

/**
 * Replace and existing workspace by finding the data source matching the
 *workspace name and updating the data source. Returns true only if replacment
 *is made.
 *
 * @param wsName : Name of the workspace to replace
 * @param matrixWorkspace : Pointer to the workspace object
 * @return : True only if a replacement was completed
 */
bool SpectrumView::replaceExistingWorkspace(
    const std::string &wsName,
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> matrixWorkspace) {

  bool replacementMade = false;

  auto existingDataSource =
      std::find_if(m_dataSource.begin(), m_dataSource.end(),
                   [&wsName](const MatrixWSDataSource_sptr &item) {
                     return wsName == item->getWorkspace()->getName();
                   });
  if (existingDataSource != m_dataSource.end()) {
    auto index = std::distance(m_dataSource.begin(), existingDataSource);
    auto targetSpectrumDisplay = m_spectrumDisplay[static_cast<int>(index)];
    // Keep the current coordinates
    const auto xPoint = targetSpectrumDisplay->getPointedAtX();
    const auto yPoint = targetSpectrumDisplay->getPointedAtY();
    auto newDataSource =
        boost::make_shared<MatrixWSDataSource>(matrixWorkspace);
    targetSpectrumDisplay->setDataSource(newDataSource);
    targetSpectrumDisplay->setPointedAtXY(xPoint, yPoint);
    // Handle range and image updates
    targetSpectrumDisplay->updateRange();
    replacementMade = true;
  }
  return replacementMade;
}

void SpectrumView::dropEvent(QDropEvent *de) {
  auto words = de->mimeData()->text().split('"');
  auto ws =
      Mantid::API::AnalysisDataService::Instance()
          .retrieveWS<Mantid::API::MatrixWorkspace>(words[1].toStdString());

  renderWorkspace(ws);
}

void SpectrumView::dragMoveEvent(QDragMoveEvent *de) {
  auto pos = m_ui->imageTabs->mapFrom(this, de->pos());
  if (m_ui->imageTabs->rect().contains(pos)) {
    de->accept();
  } else {
    de->ignore();
  }
}

void SpectrumView::dragEnterEvent(QDragEnterEvent *de) {
  if (de->mimeData()->objectName() == "MantidWorkspace") {
    de->acceptProposedAction();
  }
}

void SpectrumView::changeSpectrumDisplay(int tab) {
  auto spectrumDisplay = m_spectrumDisplay[tab].get();
  m_svConnections->setSpectrumDisplay(spectrumDisplay);
  m_hGraph->clear();
  m_vGraph->clear();
}

void SpectrumView::respondToTabCloseReqest(int tab) {
  if (m_spectrumDisplay.size() > 1) {
    auto displayToRemove = m_spectrumDisplay[tab];
    for (auto &disp : m_spectrumDisplay) {
      if (disp.get() != displayToRemove.get()) {
        (*disp).removeOther(displayToRemove);
      }
    }
    m_svConnections->removeSpectrumDisplay(displayToRemove.get());
    m_spectrumDisplay.removeAll(displayToRemove);
    m_ui->imageTabs->removeTab(tab);
    m_dataSource.erase(m_dataSource.begin() + tab);
    m_hGraph->clear();
    m_vGraph->clear();
  }
  if (m_spectrumDisplay.size() == 1) {
    m_ui->imageTabs->setTabsClosable(false);
  }
}

void SpectrumView::selectData(int spectrumNumber, double dataVal) {
  auto index = m_ui->imageTabs->currentIndex();
  auto y = static_cast<double>(spectrumNumber - 1);
  auto x = dataVal;
  m_spectrumDisplay.at(index)->setHGraph(y);
  m_spectrumDisplay.at(index)->setVGraph(x);
  m_spectrumDisplay.at(index)->showInfoList(x, y);
}

/**
 * Check if mouse tracking should be "always on".
 */
bool SpectrumView::isTrackingOn() const {
  return m_ui->tracking_always_on->isChecked();
}

API::IProjectSerialisable *
SpectrumView::loadFromProject(const std::string &lines, ApplicationWindow *app,
                              const int fileVersion) {
  UNUSED_ARG(app);
  UNUSED_ARG(fileVersion);
  API::TSVSerialiser tsv(lines);

  double min, max, step, efixed;
  int emode, intensity, graphMax;
  bool cursorTracking;
  QRect geometry;

  tsv.selectLine("geometry");
  tsv >> geometry;

  std::vector<std::string> workspaceNames;
  tsv.selectLine("Workspaces");
  tsv >> workspaceNames;

  auto viewer = new SpectrumView(nullptr);
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  for (auto &name : workspaceNames) {
    auto ws = ads.retrieveWS<Mantid::API::MatrixWorkspace>(name);
    if (ws)
      viewer->renderWorkspace(ws);
  }

  tsv.selectLine("Range");
  tsv >> min >> max >> step;
  tsv.selectLine("Intensity");
  tsv >> intensity;
  tsv.selectLine("GraphMax");
  tsv >> graphMax;
  tsv.selectLine("CursorTracking");
  tsv >> cursorTracking;
  tsv.selectLine("EMode");
  tsv >> emode;
  tsv.selectLine("EFixed");
  tsv >> efixed;

  if (tsv.selectLine("ColorMapFileName")) {
    QString fileName;
    tsv >> fileName;
    viewer->m_svConnections->loadColorMap(fileName);
  } else if (tsv.selectLine("ColorScales")) {
    int positive, negative;
    tsv >> positive >> negative;
    auto posScale = static_cast<ColorMaps::ColorScale>(positive);
    auto negScale = static_cast<ColorMaps::ColorScale>(negative);
    viewer->m_svConnections->setColorScale(posScale, negScale);
  }

  int index = viewer->m_ui->imageTabs->currentIndex();
  auto display = viewer->m_spectrumDisplay.at(index);

  double x, y;
  tsv.selectLine("SelectedPoint");
  tsv >> x >> y;

  display->setPointedAtXY(x, y);

  QPoint hPoint, vPoint;
  tsv.selectLine("HorizontalPoint");
  tsv >> hPoint;
  tsv.selectLine("VerticalPoint");
  tsv >> vPoint;

  bool hScroll, vScroll;
  tsv.selectLine("Scrolling");
  tsv >> hScroll >> vScroll;

  viewer->m_rangeHandler->setRange(min, max, step);
  viewer->m_ui->intensity_slider->setValue(intensity);
  viewer->m_ui->graph_max_slider->setValue(graphMax);
  viewer->m_ui->tracking_always_on->setChecked(cursorTracking);
  viewer->m_emodeHandler->setEMode(emode);
  viewer->m_emodeHandler->setEFixed(efixed);
  viewer->m_hGraph->setPointedAtPoint(hPoint);
  viewer->m_vGraph->setPointedAtPoint(vPoint);

  viewer->setGeometry(geometry);
  viewer->show(); // important! show before drawing/updating
  display->updateImage();

  viewer->m_ui->action_Vscroll->setChecked(vScroll);
  viewer->m_ui->action_Hscroll->setChecked(hScroll);

  return viewer;
}

std::string SpectrumView::saveToProject(ApplicationWindow *app) {
  UNUSED_ARG(app);
  API::TSVSerialiser tsv, spec;
  spec.writeLine("geometry") << geometry();

  double min, max, step;
  m_rangeHandler->getRange(min, max, step);
  spec.writeLine("Range") << min << max << step;
  spec.writeLine("Intensity") << m_ui->intensity_slider->value();
  spec.writeLine("GraphMax") << m_ui->graph_max_slider->value();
  spec.writeLine("CursorTracking") << m_ui->tracking_always_on->isChecked();
  spec.writeLine("EMode") << m_emodeHandler->getEMode();
  spec.writeLine("EFixed") << m_ui->efixed_control->text();

  auto colorScales = m_svConnections->getColorScales();
  spec.writeLine("ColorScales");
  spec << static_cast<int>(colorScales.first);
  spec << static_cast<int>(colorScales.second);

  auto colorMapFileName = m_svConnections->getColorMapFileName();
  if (!colorMapFileName.isEmpty())
    spec.writeLine("ColorMapFileName") << colorMapFileName;

  spec.writeLine("Workspaces");
  for (const auto &source : m_dataSource) {
    spec << source->getWorkspace()->getName();
  }

  int index = m_ui->imageTabs->currentIndex();
  auto display = m_spectrumDisplay.at(index);
  spec.writeLine("SelectedPoint");
  spec << display->getPointedAtX();
  spec << display->getPointedAtY();

  auto hPoint = m_hGraph->getPointedAtPoint();
  auto vPoint = m_vGraph->getPointedAtPoint();
  spec.writeLine("HorizontalPoint") << hPoint;
  spec.writeLine("VerticalPoint") << vPoint;

  auto vScroll = m_ui->action_Vscroll->isChecked();
  auto hScroll = m_ui->action_Hscroll->isChecked();
  spec.writeLine("Scrolling") << hScroll << vScroll;

  tsv.writeSection("spectrumviewer", spec.outputLines());
  return tsv.outputLines();
}

std::string SpectrumView::getWindowName() {
  return this->windowTitle().toStdString();
}

std::vector<std::string> SpectrumView::getWorkspaceNames() {
  std::vector<std::string> names;
  for (const auto &source : m_dataSource) {
    names.push_back(source->getWorkspace()->getName());
  }
  return names;
}

std::string SpectrumView::getWindowType() { return "SpectrumView"; }

void SpectrumView::changeTracking(bool on) {
  if (m_spectrumDisplay.isEmpty()) {
    return;
  }
  auto tab = m_ui->imageTabs->currentIndex();
  m_spectrumDisplay[tab]->setTrackingOn(on);
}

/// Load settings
void SpectrumView::loadSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  m_ui->tracking_always_on->setChecked(
      settings.value("CursorTracking", true).toBool());
}

/// Save settings
void SpectrumView::saveSettings() const {
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  settings.setValue("CursorTracking", m_ui->tracking_always_on->isChecked());
}

} // namespace SpectrumView
} // namespace MantidQt
