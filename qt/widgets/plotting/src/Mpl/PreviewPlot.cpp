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
 * Add a curve for a given spectrum to the plot
 * @param curveName A string label for the curve
 * @param ws A pointer to a MatrixWorkspace that contains the data
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

  const auto &histogram = ws->histogram(wsIndex);
  const auto xpts = histogram.points();
  const auto signal = histogram.y();
  auto axes = m_canvas->gca();
  m_lines.emplace_back(
      Line2DInfo{axes.plot(xpts.data().rawData(), signal.rawData()), curveName,
                 ws.get(), wsIndex});
  axes.relim();
  m_canvas->draw();
}

void PreviewPlot::addSpectrum(const QString &curveName, const QString &wsName,
                              const size_t wsIndex, const QColor &curveColour) {
  addSpectrum(curveName,
              AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
                  curveName.toStdString()),
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

void PreviewPlot::setAxisRange(const QPair<double, double> &range,
                               AxisID axisID) {
  throw std::runtime_error("setAxisRange unimplemented");
}

/**
 * Clear all curves from the plot
 */
void PreviewPlot::clear() { m_lines.clear(); }

void PreviewPlot::resizeX() {
  throw std::runtime_error("resizeX unimplemented");
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
  }
  m_canvas->draw();
}

/**
 * Observer method called when a workspace is replaced in the ADS
 * @param nf A pointer to the notification object
 */
void PreviewPlot::onWorkspaceReplaced(
    Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf) {
  // Ignore non matrix workspaces
  if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(nf->oldObject())) {
    removeLines(*ws);
  }
  m_canvas->draw();
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

} // namespace MantidWidgets
} // namespace MantidQt
