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

#include <QAction>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMenu>
#include <QVBoxLayout>

#include <algorithm>

namespace {
Mantid::Kernel::Logger g_log("PreviewPlot");
constexpr bool DRAGGABLE_LEGEND{true};
} // namespace

namespace MantidQt {
using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace;
using Widgets::MplCpp::FigureCanvasQt;
using Widgets::MplCpp::Line2D;

namespace MantidWidgets {

PreviewPlot::PreviewPlot(QWidget *parent, bool watchADS)
    : QWidget(parent), m_canvas{new FigureCanvasQt(111, parent)}, m_lines{},
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
  auto axes = m_canvas->gca();
  m_lines.emplace_back(
      createLineInfo(axes, *ws, wsIndex, lineName, lineColour));
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
    showContextMenu(static_cast<QContextMenuEvent *>(evt));
    stopEvent = true;
    break;
  default:
    break;
  }
  return stopEvent;
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
  m_contextLegend = new QAction("Legend", this);
  m_contextLegend->setCheckable(true);
  m_contextLegend->setChecked(true);
  connect(m_contextLegend, &QAction::toggled, this, &PreviewPlot::toggleLegend);
}

/**
 * @return True if the legend is visible, false otherwise
 */
bool PreviewPlot::legendIsVisible() const {
  return m_contextLegend->isChecked() && !m_lines.empty();
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
      replaceLines(*oldWS, *newWS);
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
  return Line2DInfo{createLine(axes, ws, wsIndex, lineName, lineColour), &ws,
                    wsIndex, lineName, lineColour};
}

/**
 * Add a line to the canvas using the data from the workspace
 * @param axes A reference to the axes to contain the lines
 * @param ws A reference to the workspace
 * @param wsIndex The wsIndex
 * @param label A label for the line
 * @param colour The color required for the line
 */
Line2D PreviewPlot::createLine(Widgets::MplCpp::Axes &axes,
                               const Mantid::API::MatrixWorkspace &ws,
                               const size_t wsIndex, const QString &label,
                               const QColor &lineColor) {
  const auto &histogram = ws.histogram(wsIndex);
  const auto xpts = histogram.points();
  const auto signal = histogram.y();
  return axes.plot(xpts.data().rawData(), signal.rawData(),
                   lineColor.name(QColor::HexRgb), label);
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
void PreviewPlot::replaceLines(const Mantid::API::MatrixWorkspace &oldWS,
                               const Mantid::API::MatrixWorkspace &newWS) {
  auto axes = m_canvas->gca();
  for (auto &info : m_lines) {
    if (info.workspace == &oldWS) {
      info.line =
          createLine(axes, newWS, info.wsIndex, info.label, info.colour);
      info.workspace = &newWS;
    }
  }
  regenerateLegend();
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
 * Display the context menu for the canvas
 */
void PreviewPlot::showContextMenu(QContextMenuEvent *evt) {
  QMenu contextMenu{this};
  contextMenu.addAction(m_contextLegend);
  contextMenu.exec(evt->globalPos());
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
