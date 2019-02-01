// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSurfaceContourPlotGenerator.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidPlotUtilities.h"
#include <MantidQtWidgets/Common/MantidDisplayBase.h>

using namespace MantidQt::MantidWidgets;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceGroup_const_sptr;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::HistogramData::Histogram;

/**
 * Constructor
 * @param mantidUI :: [input] Pointer to the Mantid UI
 */
MantidSurfaceContourPlotGenerator::MantidSurfaceContourPlotGenerator(
    MantidDisplayBase *mantidUI)
    : m_mantidUI(mantidUI) {}

/**
 * Plots a surface graph from the given workspace group
 * @param accepted :: [input] true if plot has been accepted
 * @param plotIndex :: [input] plot index
 * @param axisName :: [input] axis name
 * @param logName :: [input] log name
 * @param customLogValues :: [input] custom log values
 * @param workspaces :: [input] set of workspaces forming the group to be
 * plotted
 */
void MantidSurfaceContourPlotGenerator::plotSurface(
    bool accepted, int plotIndex, const QString &axisName,
    const QString &logName, const std::set<double> &customLogValues,
    const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces)
    const {
  plot(Type::Surface, accepted, plotIndex, axisName, logName, customLogValues,
       workspaces);
}

/**
 * Plots a contour plot from the given workspace group
 * @param accepted :: [input] true if plot has been accepted
 * @param plotIndex :: [input] plot index
 * @param axisName :: [input] axis name
 * @param logName :: [input] log name
 * @param customLogValues :: [input] custom log values
 * @param workspaces :: [input] set of workspaces forming the group to be
 * plotted
 */
void MantidSurfaceContourPlotGenerator::plotContour(
    bool accepted, int plotIndex, const QString &axisName,
    const QString &logName, const std::set<double> &customLogValues,
    const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces)
    const {
  plot(Type::Contour, accepted, plotIndex, axisName, logName, customLogValues,
       workspaces);
}

/**
 * Plots a contour or surface graph from the given workspace group
 * @param graphType :: [input] Type of graph to plot
 * @param accepted :: [input] true if plot has been accepted
 * @param plotIndex :: [input] plot index
 * @param axisName :: [input] axis name
 * @param logName :: [input] log name
 * @param customLogValues :: [input] custom log values
 * @param workspaces :: [input] set of workspaces forming the group to be
 * plotted
 */
void MantidSurfaceContourPlotGenerator::plot(
    Type graphType, bool accepted, int plotIndex, const QString &axisName,
    const QString &logName, const std::set<double> &customLogValues,
    const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces)
    const {
  if (!workspaces.empty() && accepted) {

    // Set up one new matrix workspace to hold all the data for plotting
    MatrixWorkspace_sptr matrixWS;
    try {
      matrixWS = createWorkspaceForGroupPlot(graphType, workspaces, plotIndex,
                                             logName, customLogValues);
    } catch (const std::logic_error &err) {
      m_mantidUI->showCritical(err.what());
      return;
    } // We can now assume every workspace is a Matrix Workspace

    // Generate X axis title
    const auto &xLabelQ = getXAxisTitle(workspaces);

    // Import the data for plotting
    auto matrixToPlot =
        m_mantidUI->importMatrixWorkspace(matrixWS, -1, -1, false);

    // Change the default plot title
    QString title =
        QString("plot for %1, spectrum %2")
            .arg(workspaces[0]->getName().c_str(), QString::number(plotIndex));
    // For the time being we use the name of the first workspace.
    // Later we need a way of conveying a name for this set of workspaces.

    // Plot the correct type of graph
    if (graphType == Type::Surface) {
      auto plot = matrixToPlot->plotGraph3D(Qwt3D::PLOTSTYLE::FILLED);
      plot->setTitle(QString("Surface ").append(title));
      plot->setXAxisLabel(xLabelQ);
      plot->setYAxisLabel(axisName);
      plot->setResolution(1); // If auto-set too high, appears empty
    } else if (graphType == Type::Contour) {
      MultiLayer *plot =
          matrixToPlot->plotGraph2D(GraphOptions::ColorMapContour);
      plot->activeGraph()->setTitle(QString("Contour ").append(title));
      plot->activeGraph()->setXAxisTitle(xLabelQ);
      plot->activeGraph()->setYAxisTitle(axisName);
    }
  }
}

/**
 * Create a workspace for the surface/contour plot from the given workspace
 * group.
 *
 * Note that only MatrixWorkspaces can be plotted, so if the group contains
 * Table or Peaks workspaces then it cannot be used.
 *
 * @param graphType :: [input] Type of graph to plot
 * @param workspaces :: [input] set of workspaces forming the group to be
 *plotted
 * @param plotIndex :: [input] plot index
 * @param logName :: [input] log name
 * @param customLogValues :: [input] custom log
 * @returns Pointer to the created workspace
 */
const MatrixWorkspace_sptr
MantidSurfaceContourPlotGenerator::createWorkspaceForGroupPlot(
    Type graphType,
    const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces,
    int plotIndex, const QString &logName,
    const std::set<double> &customLogValues) const {
  const auto index =
      static_cast<size_t>(plotIndex); // which spectrum to plot from each WS

  validateWorkspaceChoices(workspaces, index);

  // Create workspace to hold the data
  // Each "spectrum" will be the data from one workspace
  const auto nWorkspaces = workspaces.size();

  MatrixWorkspace_sptr matrixWS; // Workspace to return
  // Cast succeeds: have already checked group contains only MatrixWorkspaces
  const auto firstWS =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(workspaces[0]);

  // If we are making a surface plot, create a point data workspace.
  // If it's a contour plot, make a histo workspace.
  const auto xMode = graphType == Type::Contour ? Histogram::XMode::BinEdges
                                                : Histogram::XMode::Points;
  const auto firstBlocksize = firstWS->blocksize();
  const auto xSize =
      graphType == Type::Contour ? firstBlocksize + 1 : firstBlocksize;
  matrixWS = Mantid::API::WorkspaceFactory::Instance().create(
      firstWS, nWorkspaces, xSize, firstBlocksize);
  matrixWS->setYUnitLabel(firstWS->YUnitLabel());

  // For each workspace in group, add data and log values
  std::vector<double> logValues;
  for (size_t i = 0; i < nWorkspaces; i++) {
    const auto ws =
        boost::dynamic_pointer_cast<const MatrixWorkspace>(workspaces[i]);
    if (ws) {
      // Make sure the X data is set as the correct mode
      if (xMode == Histogram::XMode::BinEdges) {
        matrixWS->setBinEdges(i, ws->binEdges(index));
      } else {
        matrixWS->setPoints(i, ws->points(index));
      }
      // Y and E can be shared
      matrixWS->setSharedY(i, ws->sharedY(index));
      matrixWS->setSharedE(i, ws->sharedE(index));
      if (logName == MantidWSIndexWidget::CUSTOM) {
        logValues.push_back(getSingleLogValue(i, customLogValues));
      } else {
        logValues.push_back(getSingleLogValue(i, ws, logName));
      }
    }
  }

  // Set log axis values by replacing the "spectra" axis
  matrixWS->replaceAxis(1, new Mantid::API::NumericAxis(logValues));

  return matrixWS;
}

/**
 * Gets the custom, user-provided log value of the given index.
 * i.e. the nth in order from smallest to largest.
 * If the index is outside the range, returns 0.
 * (Note: MantidDock has previously checked input to make sure that there are
 * the correct number)
 * @param wsIndex :: [input] Index of log value to use
 * @param logValues :: [input] User-provided set of log values
 * @returns Numeric log value
 */
double MantidSurfaceContourPlotGenerator::getSingleLogValue(
    size_t wsIndex, const std::set<double> &logValues) const {
  return getSingleWorkspaceLogValue(wsIndex, logValues);
}

/**
 * Gets the given log value from the given workspace as a double.
 * Should be a single-valued log!
 * @param wsIndex :: [input] Index of workspace in group
 * @param matrixWS :: [input] Workspace to find log from
 * @param logName :: [input] Name of log
 * @returns log value as a double, or workspace index
 * @throws invalid_argument if log is wrong type or not present
 */
double MantidSurfaceContourPlotGenerator::getSingleLogValue(
    size_t wsIndex, const Mantid::API::MatrixWorkspace_const_sptr &matrixWS,
    const QString &logName) const {

  return getSingleWorkspaceLogValue(wsIndex, matrixWS, logName);
}

/**
 * Performs validation of user's selected options.
 * If errors detected, sets "accepted" to false and returns an error string,
 * otherwise returns an empty string.
 * Checks made:
 * - Custom values: must have same number as number of workspaces in group
 * @param options :: [input] Selections made from dialog
 * @param nWorkspaces :: [input] Number of workspaces in selected group
 * @returns Error string, or empty string if no error
 */
std::string MantidSurfaceContourPlotGenerator::validatePlotOptions(
    MantidQt::MantidWidgets::MantidWSIndexWidget::UserInputAdvanced &options,
    int nWorkspaces) {
  std::stringstream err;
  if (options.accepted) {
    if (options.logName == MantidWSIndexWidget::CUSTOM) {
      // Check number of values supplied
      if (static_cast<int>(options.customLogValues.size()) != nWorkspaces) {
        err << "Number of custom log values must be equal to "
               "number of workspaces in group";
        options.accepted = false;
      }
    }
  }
  return err.str();
}
// This function is not currently called.
// May be needed later on in the GUI harmonization.

/**
 * Generates X axis title for graph based on first workspace in group
 * @param workspaces :: [input] list of workpaces containing data for graph.
 * The title is generated from the first of these workspaces.
 * @returns :: Title for X axis of graph
 */
QString MantidSurfaceContourPlotGenerator::getXAxisTitle(
    const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces)
    const {
  if (workspaces.empty()) {
    return QString();
  }
  const auto firstWS =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(workspaces[0]);
  // Already checked group contains only MatrixWorkspaces
  const auto &xAxisLabel = firstWS->getXDimension()->getName();
  const auto &xAxisUnits = firstWS->getXDimension()->getUnits().ascii();
  // Generate title for the X axis
  QString xAxisTitle = xAxisLabel.empty() ? "X" : xAxisLabel.c_str();
  if (!xAxisUnits.empty()) {
    xAxisTitle.append(" (").append(xAxisUnits.c_str()).append(")");
  }
  return xAxisTitle;
}

/**
 * Test if all workspaces in the group have the same X data for the given
 * spectrum.
 * (At the moment just tests size of X data)
 * Precondition: wsGroup contains only MatrixWorkspaces
 *
 * @param workspaces :: [input] Workspaces to test
 * @param index :: [input] Index of spectrum to test
 * @return :: True if X data same, else false.
 * @throw std::logic_error if spectrum index not contained in workspace, or if
 * wsGroup contains workspaces other than MatrixWorkspaces
 */
bool MantidSurfaceContourPlotGenerator::groupContentsHaveSameX(
    const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces,
    const size_t index) {
  if (workspaces.empty()) {
    return false;
  }

  // Check and retrieve X data for given workspace, spectrum
  const auto getXData = [&workspaces](const size_t index,
                                      const size_t spectrum) {
    const auto &ws =
        boost::dynamic_pointer_cast<const MatrixWorkspace>(workspaces[index]);
    if (ws) {
      if (ws->getNumberHistograms() < spectrum) {
        throw std::logic_error("Spectrum index too large for some workspaces");
      } else {
        return ws->x(spectrum);
      }
    } else {
      throw std::logic_error(
          "Group contains something other than MatrixWorkspaces");
    }
  };

  const auto nWorkspaces = workspaces.size();
  switch (nWorkspaces) {
  case 0:
    return false;
  case 1:
    return true; // all spectra (only 1) have same X
  default:
    bool allSameX = true;
    const auto &firstX = getXData(0, index);
    for (size_t i = 1; i < nWorkspaces; ++i) {
      const auto &x = getXData(i, index);
      if (x.size() != firstX.size()) {
        allSameX = false;
        break;
      }
    }
    return allSameX;
  }
}

/**
 * Validate the supplied workspaces and spectrum index.
 * - Group must not be empty
 * - Group must only contain MatrixWorkspaces
 * - Group must have same X data for all workspaces
 * @param workspaces :: [input] Workspaces to test
 * @param spectrum :: [input] Spectrum index to test
 * @throws std::invalid_argument if validation fails.
 */
void MantidSurfaceContourPlotGenerator::validateWorkspaceChoices(
    const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces,
    const size_t spectrum) const {
  if (workspaces.empty()) {
    throw std::invalid_argument("Must provide a non-empty WorkspaceGroup");
  }

  if (!groupContentsHaveSameX(workspaces, spectrum)) {
    throw std::invalid_argument(
        "Input WorkspaceGroup must have same X data for all workspaces");
  }
}
