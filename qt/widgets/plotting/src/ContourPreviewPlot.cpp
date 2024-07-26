// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/ContourPreviewPlot.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/MantidAxes.h"

#include <QVBoxLayout>

using namespace Mantid::API;
using namespace MantidQt::Widgets::MplCpp;

namespace {
Mantid::Kernel::Logger g_log("ContourPreviewPlot");
constexpr auto MANTID_PROJECTION = "mantid";
} // namespace

namespace MantidQt::MantidWidgets {

ContourPreviewPlot::ContourPreviewPlot(QWidget *parent, bool observeADS)
    : QWidget(parent), m_canvas(new FigureCanvasQt(111, MANTID_PROJECTION, parent)) {
  createLayout();
  watchADS(observeADS);
}

/**
 * Initialize the layout for the widget
 */
void ContourPreviewPlot::createLayout() {
  auto plotLayout = new QVBoxLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  plotLayout->addWidget(m_canvas, 0);
  setLayout(plotLayout);
}

/**
 * Enable/disable the ADS observers
 * @param on If true ADS observers are enabled else they are disabled
 */
void ContourPreviewPlot::watchADS(bool on) {
  this->observeReplace(on);
  this->observeDelete(on);
}

/**
 * Observer method called when a workspace is removed from the ADS
 * @param nf A pointer to the notification object
 */
void ContourPreviewPlot::deleteHandle(const std::string &wsName, const Workspace_sptr &workspace) {
  (void)wsName;
  if (auto matrixWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(workspace)) {
    // If the artist has already been removed, ignore.
    bool workspaceRemoved = false;
    try {
      workspaceRemoved = m_canvas->gca<MantidAxes>().removeWorkspaceArtists(matrixWorkspace);
    } catch (Mantid::PythonInterface::PythonException &) {
    }
    if (workspaceRemoved) {
      m_canvas->draw();
    }
  }
}

/**
 * Observer method called when a workspace is replaced in the ADS
 * @param nf A pointer to the notification object
 */
void ContourPreviewPlot::replaceHandle(const std::string &wsName, const Workspace_sptr &workspace) {
  (void)wsName;
  if (auto newWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(workspace)) {
    if (m_canvas->gca<MantidAxes>().replaceWorkspaceArtists(newWorkspace)) {
      m_canvas->draw();
    }
  }
}

/**
 * Clears current contour plot held in the MantidAxes
 */
void ContourPreviewPlot::clearPlot() { m_canvas->gca<MantidAxes>().clear(); }

/**
 * Set the face colour for the canvas
 * @param colour A new colour for the figure facecolor
 */
void ContourPreviewPlot::setCanvasColour(QColor const &colour) { m_canvas->gcf().setFaceColor(colour); }

/**
 * Sets the workspace for the contour plot
 * @param workspace The workspace to plot on the contour plot.
 */
void ContourPreviewPlot::setWorkspace(const MatrixWorkspace_sptr &workspace) {
  if (workspace) {
    auto axes = m_canvas->gca<MantidAxes>();
    axes.pcolormesh(workspace);
    m_canvas->draw();
  } else {
    g_log.warning("Cannot plot a null workspace.");
  }
}

/**
 * Gets the range of the supplied axis
 * @param axisID The axis to get the range for
 * @return The axis range
 */
std::tuple<double, double> ContourPreviewPlot::getAxisRange(AxisID axisID) const {
  switch (axisID) {
  case AxisID::XBottom:
    return m_canvas->gca().getXLim();
  case AxisID::YLeft:
    return m_canvas->gca().getYLim();
  }
  throw std::runtime_error("Incorrect AxisID provided. Axis types are XBottom and YLeft");
}

} // namespace MantidQt::MantidWidgets
