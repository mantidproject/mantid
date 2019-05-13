// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/MantidAxes.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMenu>
#include <QVBoxLayout>

#include <algorithm>

using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace;
using MantidQt::Widgets::MplCpp::Figure;
using MantidQt::Widgets::MplCpp::FigureCanvasQt;
using MantidQt::Widgets::MplCpp::Line2D;
using MantidQt::Widgets::MplCpp::MantidAxes;

namespace {
Mantid::Kernel::Logger g_log("PreviewPlot");
constexpr auto MANTID_PROJECTION{"mantid"};
constexpr auto DRAGGABLE_LEGEND{true};
constexpr auto PLOT_TOOL_NONE{"None"};
constexpr auto PLOT_TOOL_PAN{"Pan"};
constexpr auto PLOT_TOOL_ZOOM{"Zoom"};
constexpr auto LINEAR_SCALE{"Linear"};
constexpr auto LOG_SCALE{"Log"};
constexpr auto SQUARE_SCALE{"Square"};

/**
 * Construct the line data from a workspace
 * @param ws A reference to the workspace to plot
 * @param wsIndex A wsIndex denoting the spectrum to plot
 * @return A new Line2D::Data object
 */
Line2D::Data tolineData(const Mantid::API::MatrixWorkspace &ws,
                        const size_t wsIndex) {
  const auto &histogram = ws.histogram(wsIndex);
  return {histogram.points().rawData(), histogram.y().rawData()};
}

} // namespace

namespace MantidQt {

namespace MantidWidgets {

/**
 * Construct a plot object
 * @param parent The parent widget
 * @param watchADS If true then ADS observers are added
 */
PreviewPlot::PreviewPlot(QWidget *parent, bool watchADS)
    : QWidget(parent), m_canvas{new FigureCanvasQt(111, "mantid", parent)},
      m_lines{}, m_panZoomTool(m_canvas),
      m_wsRemovedObserver(*this, &PreviewPlot::onWorkspaceRemoved),
      m_wsReplacedObserver(*this, &PreviewPlot::onWorkspaceReplaced) {
  createLayout();
  createActions();

  m_canvas->installEventFilterToMplCanvas(this);
  if (watchADS) {
    auto &ads = AnalysisDataService::Instance();
    ads.notificationCenter.addObserver(m_wsRemovedObserver);
    ads.notificationCenter.addObserver(m_wsReplacedObserver);
  }
}

/**
 * Destructor.
 * Remove ADS observers
 */
PreviewPlot::~PreviewPlot() {
  auto &notificationCenter = AnalysisDataService::Instance().notificationCenter;
  notificationCenter.removeObserver(m_wsReplacedObserver);
  notificationCenter.removeObserver(m_wsRemovedObserver);
}

/**
 * Add a line for a given spectrum to the plot
 * @param lineName A string label for the line
 * @param ws A MatrixWorkspace that contains the data
 * @param wsIndex The index of the workspace to access
 * @param lineColour Defines the color of the line
 */
void PreviewPlot::addSpectrum(const QString &lineName,
                              const Mantid::API::MatrixWorkspace_sptr &ws,
                              const size_t wsIndex, const QColor &lineColour) {
  if (lineName.isEmpty()) {
    g_log.warning("Cannot plot with empty line name");
    return;
  }
  if (!ws) {
    g_log.warning("Cannot plot null workspace");
    return;
  }
  removeSpectrum(lineName);
  auto axes = m_canvas->gca<MantidAxes>();
  axes.plot(ws, wsIndex, lineColour.name(QColor::HexRgb), lineName);
  //  m_lines.emplace_back(
  //      createLineInfo(axes, *ws, wsIndex, lineName, lineColour));
  regenerateLegend();
  axes.relim();
  m_canvas->draw();
}

/**
 * Add a line for a given spectrum to the plot
 * @param lineName A string label for the line
 * @param wsName A name of a MatrixWorkspace that contains the data
 * @param wsIndex The index of the workspace to access
 * @param lineColour Defines the color of the line
 */
void PreviewPlot::addSpectrum(const QString &lineName, const QString &wsName,
                              const size_t wsIndex, const QColor &lineColour) {
  addSpectrum(lineName,
              AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                  wsName.toStdString()),
              wsIndex, lineColour);
}

/**
 * Remove the named line from the plot
 * @param lineName A name of a given line on the plot. If the lineName is
 * not known then this does nothing
 */
void PreviewPlot::removeSpectrum(const QString &lineName) {
  auto lineIter = std::find_if(
      m_lines.cbegin(), m_lines.cend(),
      [&lineName](const auto &line) { return line.label == lineName; });
  if (lineIter != m_lines.cend()) {
    m_lines.erase(lineIter);
    regenerateLegend();
  }
}

/**
 * Set the range of the specified axis
 * @param range The new range
 * @param axisID An enumeration defining the axis
 */
void PreviewPlot::setAxisRange(const QPair<double, double> &range,
                               AxisID axisID) {
  switch (axisID) {
  case AxisID::XBottom:
    m_canvas->gca().setXLim(range.first, range.second);
    break;
  case AxisID::YLeft:
    m_canvas->gca().setYLim(range.first, range.second);
    break;
  }
}

/**
 * Clear all lines from the plot
 */
void PreviewPlot::clear() {
  m_lines.clear();
  regenerateLegend();
}

/**
 * Resize the X axis to encompass all of the data
 */
void PreviewPlot::resizeX() { m_canvas->gca().autoscaleView(true, false); }

/**
 * Reset the whole view to show all of the data
 */
void PreviewPlot::resetView() { m_panZoomTool.zoomOut(); }

/**
 * Toggle for programatic legend visibility toggle
 * @param visible If True the legend is visible on the canvas
 */
void PreviewPlot::showLegend(const bool visible) {
  m_contextLegend->setChecked(visible);
}

/**
 * Capture events destined for the canvas
 * @param watched Target object (Unused)
 * @param evt A pointer to the event object
 * @return True if the event should be stopped, false otherwise
 */
bool PreviewPlot::eventFilter(QObject *watched, QEvent *evt) {
  Q_UNUSED(watched);
  bool stopEvent{false};
  switch (evt->type()) {
  case QEvent::ContextMenu:
    // handled by mouse press events below as we need to
    // stop the canvas getting mouse events in some circumstances
    // to disable zooming/panning
    stopEvent = true;
    break;
  case QEvent::MouseButtonPress:
    stopEvent = handleMousePressEvent(static_cast<QMouseEvent *>(evt));
    break;
  case QEvent::MouseButtonRelease:
    stopEvent = handleMouseReleaseEvent(static_cast<QMouseEvent *>(evt));
    break;
  default:
    break;
  }
  return stopEvent;
}

/**
 * Handler called when the event filter recieves a mouse press event
 * @param evt A pointer to the event
 * @return True if the event propagation should be stopped, false otherwise
 */
bool PreviewPlot::handleMousePressEvent(QMouseEvent *evt) {
  bool stopEvent(false);
  // right-click events are reserved for the context menu
  // show when the mouse click is released
  if (evt->buttons() & Qt::RightButton) {
    stopEvent = true;
  }
  return stopEvent;
}

/**
 * Handler called when the event filter recieves a mouse release event
 * @param evt A pointer to the event
 * @return True if the event propagation should be stopped, false otherwise
 */
bool PreviewPlot::handleMouseReleaseEvent(QMouseEvent *evt) {
  bool stopEvent(false);
  if (evt->button() == Qt::RightButton) {
    stopEvent = true;
    showContextMenu(evt);
  }
  return stopEvent;
}

/**
 * Display the context menu for the canvas
 */
void PreviewPlot::showContextMenu(QMouseEvent *evt) {
  QMenu contextMenu{this};
  auto plotTools = contextMenu.addMenu("Plot Tools");
  plotTools->addActions(m_contextPlotTools->actions());
  contextMenu.addAction(m_contextResetView);

  contextMenu.addSeparator();
  auto xscale = contextMenu.addMenu("X Scale");
  xscale->addActions(m_contextXScale->actions());
  auto yScale = contextMenu.addMenu("Y Scale");
  yScale->addActions(m_contextYScale->actions());

  contextMenu.addSeparator();
  contextMenu.addAction(m_contextLegend);

  contextMenu.exec(evt->globalPos());
}

/**
 * Initialize the layout for the widget
 */
void PreviewPlot::createLayout() {
  auto plotLayout = new QVBoxLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  plotLayout->addWidget(m_canvas, 0, 0);
  setLayout(plotLayout);
}

/**
 * Create the menu actions items
 */
void PreviewPlot::createActions() {
  // Create an exclusive group of checkable actions with
  auto createExclusiveActionGroup =
      [this](const std::initializer_list<const char *> &names) {
        auto group = new QActionGroup(this);
        group->setExclusive(true);
        for (const auto &name : names) {
          auto action = group->addAction(name);
          action->setCheckable(true);
        }
        group->actions()[0]->setChecked(true);
        return group;
      };
  // plot tools
  m_contextPlotTools = createExclusiveActionGroup(
      {PLOT_TOOL_NONE, PLOT_TOOL_PAN, PLOT_TOOL_ZOOM});
  connect(m_contextPlotTools, &QActionGroup::triggered, this,
          &PreviewPlot::switchPlotTool);
  m_contextResetView = new QAction("Reset Plot", this);
  connect(m_contextResetView, &QAction::triggered, this,
          &PreviewPlot::resetView);

  // scales
  m_contextXScale =
      createExclusiveActionGroup({LINEAR_SCALE, LOG_SCALE, SQUARE_SCALE});
  connect(m_contextXScale, &QActionGroup::triggered, this,
          &PreviewPlot::setXScaleType);
  m_contextYScale = createExclusiveActionGroup({LINEAR_SCALE, LOG_SCALE});
  connect(m_contextYScale, &QActionGroup::triggered, this,
          &PreviewPlot::setYScaleType);
  m_contextXScale->actions()[0]->setChecked(true);
  m_contextYScale->actions()[0]->setChecked(true);

  // legend
  m_contextLegend = new QAction("Legend", this);
  m_contextLegend->setCheckable(true);
  m_contextLegend->setChecked(true);
  connect(m_contextLegend, &QAction::toggled, this, &PreviewPlot::toggleLegend);
}

/**
 * @return True if the legend is visible, false otherwise
 */
bool PreviewPlot::legendIsVisible() const {
  return m_contextLegend->isChecked();
}

/**
 * Observer method called when a workspace is removed from the ADS
 * @param nf A pointer to the notification object
 */
void PreviewPlot::onWorkspaceRemoved(
    Mantid::API::WorkspacePreDeleteNotification_ptr nf) {
  // Ignore non matrix workspaces
  if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(nf->object())) {
    removeLines(*ws);
    m_canvas->draw();
  }
}

/**
 * Observer method called when a workspace is replaced in the ADS
 * @param nf A pointer to the notification object
 */
void PreviewPlot::onWorkspaceReplaced(
    Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf) {
  // Ignore non matrix workspaces
  if (auto oldWS =
          boost::dynamic_pointer_cast<MatrixWorkspace>(nf->oldObject())) {
    if (auto newWS =
            boost::dynamic_pointer_cast<MatrixWorkspace>(nf->newObject())) {
      replaceLineData(*oldWS, *newWS);
      m_canvas->draw();
    }
  }
}

/**
 * Add a line to the canvas using the data from the workspace and return a
 * struct describing the line
 * @param axes A reference to the axes to contain the lines
 * @param ws A reference to the workspace
 * @param wsIndex The wsIndex
 * @param lineLabel A label for the line
 * @param lineColour The color required for the line
 * @return A Line2DInfo describing the line
 */
PreviewPlot::Line2DInfo PreviewPlot::createLineInfo(
    Widgets::MplCpp::Axes &axes, const Mantid::API::MatrixWorkspace &ws,
    const size_t wsIndex, const QString &lineName, const QColor &lineColour) {
  auto data = tolineData(ws, wsIndex);
  return Line2DInfo{axes.plot(std::move(data.xaxis), std::move(data.yaxis),
                              lineColour.name(QColor::HexRgb), lineName),
                    &ws, wsIndex, lineName, lineColour};
}

/**
 * Remove all lines based on a given workspace
 * @param ws A reference to a workspace
 */
void PreviewPlot::removeLines(const Mantid::API::MatrixWorkspace &ws) {
  decltype(m_lines.cbegin()) lineIter{m_lines.cbegin()};
  while (lineIter != m_lines.cend()) {
    lineIter =
        std::find_if(m_lines.cbegin(), m_lines.cend(),
                     [&ws](const auto &info) { return info.workspace == &ws; });
    if (lineIter != m_lines.cend()) {
      m_lines.erase(lineIter);
    }
  };
  regenerateLegend();
}

/**
 * Replace any lines based on the old workspace with data from the new
 * workspace
 * @param oldWS A reference to the existing workspace
 * @param newWS A reference to the replacement workspace
 */
void PreviewPlot::replaceLineData(const Mantid::API::MatrixWorkspace &oldWS,
                                  const Mantid::API::MatrixWorkspace &newWS) {
  auto axes = m_canvas->gca();
  for (auto &info : m_lines) {
    if (info.workspace == &oldWS) {
      info.line.setData(tolineData(newWS, info.wsIndex));
      info.workspace = &newWS;
    }
  }
  axes.relim();
  m_canvas->draw();
}

/**
 * If the legend is visible regenerate it based on the current content
 */
void PreviewPlot::regenerateLegend() {
  if (legendIsVisible()) {
    m_canvas->gca().legend(DRAGGABLE_LEGEND);
  }
}

/**
 * If the legend is visible remove it from the canvas
 */
void PreviewPlot::removeLegend() {
  auto legend{m_canvas->gca().legendInstance()};
  if (!legend.pyobj().is_none()) {
    m_canvas->gca().legendInstance().remove();
  }
}

/**
 * Called when a different plot tool is selected. Enables the
 * appropriate mode on the canvas
 * @param selected A QAction pointer denoting the desired tool
 */
void PreviewPlot::switchPlotTool(QAction *selected) {
  QString toolName = selected->text();
  if (toolName == PLOT_TOOL_NONE) {
    m_panZoomTool.enableZoom(false);
    m_panZoomTool.enablePan(false);
  } else if (toolName == PLOT_TOOL_PAN) {
    m_panZoomTool.enablePan(false);
  } else if (toolName == PLOT_TOOL_ZOOM) {
    m_panZoomTool.enableZoom(true);
  } else {
    // if a tool is added to the menu but no handler is added
    g_log.warning("Unknown plot tool selected.");
  }
}

/**
 * Set the X scale based on the given QAction
 * @param selected The action that triggered the slot
 */
void PreviewPlot::setXScaleType(QAction *selected) {
  setScaleType(AxisID::XBottom, selected->text());
}

/**
 * Set the X scale based on the given QAction
 * @param selected The action that triggered the slot
 */
void PreviewPlot::setYScaleType(QAction *selected) {
  setScaleType(AxisID::YLeft, selected->text());
}

void PreviewPlot::setScaleType(AxisID id, QString actionName) {
  auto scaleType = actionName.toLower().toLatin1();
  auto axes = m_canvas->gca();
  switch (id) {
  case AxisID::XBottom:
    axes.setXScale(scaleType.constData());
    break;
  case AxisID::YLeft:
    axes.setYScale(scaleType.constData());
    break;
  default:
    break;
  }
  m_canvas->draw();
}

/**
 * Toggle the legend visibility state
 * @param checked True if the state should be visible, false otherwise
 */
void PreviewPlot::toggleLegend(const bool checked) {
  if (checked) {
    regenerateLegend();
  } else {
    removeLegend();
  }
  m_canvas->draw();
}

} // namespace MantidWidgets
} // namespace MantidQt
