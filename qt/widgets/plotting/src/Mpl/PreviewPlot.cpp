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

#include <QVBoxLayout>

#include <algorithm>

namespace {
Mantid::Kernel::Logger g_log("PreviewPlot");
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
  auto plotLayout = new QVBoxLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  plotLayout->addWidget(m_canvas, 0, 0);
  setLayout(plotLayout);

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
 * Add a curve for a given spectrum to the plot
 * @param curveName A string label for the curve
 * @param ws A MatrixWorkspace that contains the data
 * @param wsIndex The index of the workspace to access
 * @param curveColour Defines the color of the curve
 */
void PreviewPlot::addSpectrum(const QString &curveName,
                              const Mantid::API::MatrixWorkspace_sptr &ws,
                              const size_t wsIndex, const QColor &curveColour) {
  if (curveName.isEmpty()) {
    g_log.warning("Cannot plot with empty curve name");
    return;
  }
  if (!ws) {
    g_log.warning("Cannot plot null workspace");
    return;
  }
  removeSpectrum(curveName);
  auto axes = m_canvas->gca();
  m_lines.emplace_back(Line2DInfo{createLine(axes, *ws, wsIndex, curveColour),
                                  curveName, ws.get(), wsIndex, curveColour});
  m_canvas->draw();
  axes.relim();
}

/**
 * Add a curve for a given spectrum to the plot
 * @param curveName A string label for the curve
 * @param wsName A name of a MatrixWorkspace that contains the data
 * @param wsIndex The index of the workspace to access
 * @param curveColour Defines the color of the curve
 */
void PreviewPlot::addSpectrum(const QString &curveName, const QString &wsName,
                              const size_t wsIndex, const QColor &curveColour) {
  addSpectrum(curveName,
              AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                  wsName.toStdString()),
              wsIndex, curveColour);
}

/**
 * Remove the named curve from the plot
 * @param curveName A name of a given curve on the plot. If the curveName is
 * not known then this does nothing
 */
void PreviewPlot::removeSpectrum(const QString &curveName) {
  auto curveIter = std::find_if(
      m_lines.cbegin(), m_lines.cend(),
      [&curveName](const auto &curve) { return curve.name == curveName; });
  if (curveIter != m_lines.cend()) {
    m_lines.erase(curveIter);
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
 * Clear all curves from the plot
 */
void PreviewPlot::clear() { m_lines.clear(); }

/**
 * Resize the X axis to encompass all of the data
 */
void PreviewPlot::resizeX() { m_canvas->gca().autoscaleView(true, false); }

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
 * Add a line to the canvas using the data from the workspace
 * @param axes A reference to the axes to contain the lines
 * @param ws A reference to the workspace
 * @param wsIndex The wsIndex
 * @param lineColor The color required for the line
 */
Line2D PreviewPlot::createLine(Widgets::MplCpp::Axes &axes,
                               const Mantid::API::MatrixWorkspace &ws,
                               const size_t wsIndex, const QColor &lineColor) {
  const auto &histogram = ws.histogram(wsIndex);
  const auto xpts = histogram.points();
  const auto signal = histogram.y();
  const QString hexColor = lineColor.name(QColor::HexRgb);
  return axes.plot(xpts.data().rawData(), signal.rawData(),
                   hexColor.toLatin1().constData());
}

/**
 * Remove all curves based on a given workspace
 * @param ws A reference to a workspace
 */
void PreviewPlot::removeLines(const Mantid::API::MatrixWorkspace &ws) {
  decltype(m_lines.cbegin()) curveIter{m_lines.cbegin()};
  while (curveIter != m_lines.cend()) {
    curveIter =
        std::find_if(m_lines.cbegin(), m_lines.cend(),
                     [&ws](const auto &info) { return info.workspace == &ws; });
    if (curveIter != m_lines.cend()) {
      m_lines.erase(curveIter);
    }
  };
}

/**
 * Replace any curves based on the old workspace with data from the new
 * workspace
 * @param oldWS A reference to the existing workspace
 * @param newWS A reference to the replacement workspace
 */
void PreviewPlot::replaceLines(const Mantid::API::MatrixWorkspace &oldWS,
                               const Mantid::API::MatrixWorkspace &newWS) {
  auto axes = m_canvas->gca();
  for (auto &info : m_lines) {
    if (info.workspace == &oldWS) {
      info.line = createLine(axes, newWS, info.wsIndex, info.curveColour);
      info.workspace = &newWS;
    }
  }
  axes.relim();
  m_canvas->draw();
}

} // namespace MantidWidgets
} // namespace MantidQt
