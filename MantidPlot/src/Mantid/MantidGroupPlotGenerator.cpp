#include "MantidGroupPlotGenerator.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <MantidQtMantidWidgets/MantidDisplayBase.h>

using namespace MantidQt::MantidWidgets;
using Mantid::API::WorkspaceGroup_const_sptr;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::ExperimentInfo;
using Mantid::HistogramData::Histogram;

/**
 * Constructor
 * @param mantidUI :: [input] Pointer to the Mantid UI
 */
MantidGroupPlotGenerator::MantidGroupPlotGenerator(MantidDisplayBase *mantidUI)
    : m_mantidUI(mantidUI) {}

/**
 * Plots a surface graph from the given workspace group
 * @param wsGroup :: [input] Workspace group to plot
 * @param options :: [input] User-selected plot options
 */
void MantidGroupPlotGenerator::plotSurface(
    const WorkspaceGroup_const_sptr &wsGroup,
    const MantidSurfacePlotDialog::UserInputSurface &options) const {
  plot(Type::Surface, wsGroup, options);
}

/**
 * Plots a contour plot from the given workspace group
 * @param wsGroup :: [input] Workspace group to plot
 * @param options :: [input] User-selected plot options
 */
void MantidGroupPlotGenerator::plotContour(
    const WorkspaceGroup_const_sptr &wsGroup,
    const MantidSurfacePlotDialog::UserInputSurface &options) const {
  plot(Type::Contour, wsGroup, options);
}

/**
 * Plots a graph from the given workspace group
 * @param graphType :: [input] Type of graph to plot
 * @param wsGroup :: [input] Workspace group to plot
 * @param options :: [input] User-selected plot options
 */
void MantidGroupPlotGenerator::plot(
    Type graphType, const WorkspaceGroup_const_sptr &wsGroup,
    const MantidSurfacePlotDialog::UserInputSurface &options) const {
  if (wsGroup && options.accepted) {
    // Set up one new matrix workspace to hold all the data for plotting
    MatrixWorkspace_sptr matrixWS;
    try {
      matrixWS = createWorkspaceForGroupPlot(graphType, wsGroup, options);
    } catch (const std::logic_error &err) {
      m_mantidUI->showCritical(err.what());
      return;
    }

    // Generate X axis title
    const auto &xLabelQ = getXAxisTitle(wsGroup);

    // Import the data for plotting
    auto matrixToPlot =
        m_mantidUI->importMatrixWorkspace(matrixWS, -1, -1, false);

    // Change the default plot title
    QString title =
        QString("plot for %1, spectrum %2")
            .arg(wsGroup->name().c_str(), QString::number(options.plotIndex));

    // Plot the correct type of graph
    if (graphType == Type::Surface) {
      auto plot = matrixToPlot->plotGraph3D(Qwt3D::PLOTSTYLE::FILLED);
      plot->setTitle(QString("Surface ").append(title));
      plot->setXAxisLabel(xLabelQ);
      plot->setYAxisLabel(options.axisName);
      plot->setResolution(1); // If auto-set too high, appears empty
    } else if (graphType == Type::Contour) {
      MultiLayer *plot =
          matrixToPlot->plotGraph2D(GraphOptions::ColorMapContour);
      plot->activeGraph()->setXAxisTitle(xLabelQ);
      plot->activeGraph()->setYAxisTitle(options.axisName);
      plot->activeGraph()->setTitle(QString("Contour ").append(title));
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
 * @param wsGroup :: [input] Pointer to workspace group to use as input
 * @param options :: [input] User input from dialog
 * @returns Pointer to the created workspace
 */
const MatrixWorkspace_sptr
MantidGroupPlotGenerator::createWorkspaceForGroupPlot(
    Type graphType, WorkspaceGroup_const_sptr wsGroup,
    const MantidSurfacePlotDialog::UserInputSurface &options) const {
  const auto index = static_cast<size_t>(
      options.plotIndex);                // which spectrum to plot from each WS
  const auto &logName = options.logName; // Log to read for axis of XYZ plot

  validateWorkspaceChoices(wsGroup, index);

  // Create workspace to hold the data
  // Each "spectrum" will be the data from one workspace
  const auto nWorkspaces = wsGroup->getNumberOfEntries();
  if (nWorkspaces < 0) {
    return MatrixWorkspace_sptr();
  }

  MatrixWorkspace_sptr matrixWS; // Workspace to return
  // Cast succeeds: have already checked group contains only MatrixWorkspaces
  const auto firstWS =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(wsGroup->getItem(0));

  // If we are making a surface plot, create a point data workspace.
  // If it's a contour plot, make a histo workspace.
  const auto xMode = graphType == Type::Contour ? Histogram::XMode::BinEdges
                                                : Histogram::XMode::Points;
  const auto xSize = graphType == Type::Contour ? firstWS->blocksize() + 1
                                                : firstWS->blocksize();
  matrixWS = Mantid::API::WorkspaceFactory::Instance().create(
      firstWS, nWorkspaces, xSize, firstWS->blocksize());
  matrixWS->setYUnitLabel(firstWS->YUnitLabel());

  // For each workspace in group, add data and log values
  std::vector<double> logValues;
  for (int i = 0; i < nWorkspaces; i++) {
    const auto ws =
        boost::dynamic_pointer_cast<const MatrixWorkspace>(wsGroup->getItem(i));
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
      if (logName == MantidSurfacePlotDialog::CUSTOM) {
        logValues.push_back(getSingleLogValue(i, options.customLogValues));
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
 * Check if the supplied group contains only MatrixWorkspaces
 * @param wsGroup :: [input] Pointer to a WorkspaceGroup
 * @returns True if contains only MatrixWorkspaces, false if contains
 * other types or is empty
 */
bool MantidGroupPlotGenerator::groupIsAllMatrixWorkspaces(
    const WorkspaceGroup_const_sptr &wsGroup) {
  bool allMatrixWSes = true;
  if (wsGroup) {
    if (wsGroup->isEmpty()) {
      allMatrixWSes = false;
    } else {
      for (int index = 0; index < wsGroup->getNumberOfEntries(); index++) {
        if (nullptr == boost::dynamic_pointer_cast<MatrixWorkspace>(
                           wsGroup->getItem(index))) {
          allMatrixWSes = false;
          break;
        }
      }
    }
  } else {
    allMatrixWSes = false;
  }
  return allMatrixWSes;
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
double MantidGroupPlotGenerator::getSingleLogValue(
    int wsIndex, const std::set<double> &logValues) const {
  double value = 0;
  if (wsIndex < static_cast<int>(logValues.size())) {
    auto it = logValues.begin();
    std::advance(it, wsIndex);
    value = *it;
  }
  return value;
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
double MantidGroupPlotGenerator::getSingleLogValue(
    int wsIndex, const Mantid::API::MatrixWorkspace_const_sptr &matrixWS,
    const QString &logName) const {
  if (logName == MantidSurfacePlotDialog::WORKSPACE_INDEX) {
    return wsIndex;
  } else {
    // MatrixWorkspace is an ExperimentInfo
    if (auto ei = boost::dynamic_pointer_cast<const ExperimentInfo>(matrixWS)) {
      auto log = ei->run().getLogData(logName.toStdString());
      if (log) {
        if (dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(log) ||
            dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(log)) {
          return std::stod(log->value());
        } else {
          throw std::invalid_argument(
              "Log is of wrong type (expected single numeric value");
        }
      } else {
        throw std::invalid_argument("Log not present in workspace");
      }
    } else {
      throw std::invalid_argument("Bad input workspace type");
    }
  }
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
std::string MantidGroupPlotGenerator::validatePlotOptions(
    MantidSurfacePlotDialog::UserInputSurface &options, int nWorkspaces) {
  std::stringstream err;
  if (options.accepted) {
    if (options.logName == MantidSurfacePlotDialog::CUSTOM) {
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

/**
 * Generates X axis title for graph based on first workspace in group
 * @param wsGroup :: [input] WorkspaceGroup that contains data for graph - title
 * will be generated from the X label of the first workspace in the group
 * @returns :: Title for X axis of graph
 */
QString MantidGroupPlotGenerator::getXAxisTitle(
    const boost::shared_ptr<const Mantid::API::WorkspaceGroup> wsGroup) const {
  if (wsGroup->getNumberOfEntries() <= 0) {
    return QString();
  }
  const auto firstWS =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(wsGroup->getItem(
          0)); // Already checked group contains only MatrixWorkspaces
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
 * @param wsGroup :: [input] Group to test
 * @param index :: [input] Index of spectrum to test
 * @return :: True if X data same, else false.
 * @throw std::logic_error if spectrum index not contained in workspace, or if
 * wsGroup contains workspaces other than MatrixWorkspaces
 */
bool MantidGroupPlotGenerator::groupContentsHaveSameX(
    const Mantid::API::WorkspaceGroup_const_sptr &wsGroup, const size_t index) {
  if (!wsGroup) {
    return false;
  }

  // Check and retrieve X data for given workspace, spectrum
  const auto getXData = [&wsGroup](const size_t workspace,
                                   const size_t spectrum) {
    const auto &ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        wsGroup->getItem(workspace));
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

  const auto nWorkspaces = wsGroup->size();
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
 * Validate the supplied workspace group and spectrum index.
 * - Group must not be empty
 * - Group must only contain MatrixWorkspaces
 * - Group must have same X data for all workspaces
 * @param wsGroup :: [input] Workspace group to test
 * @param spectrum :: [input] Spectrum index to test
 * @throws std::invalid_argument if validation fails.
 */
void MantidGroupPlotGenerator::validateWorkspaceChoices(
    const boost::shared_ptr<const Mantid::API::WorkspaceGroup> wsGroup,
    const size_t spectrum) const {
  if (!wsGroup || wsGroup->size() == 0) {
    throw std::invalid_argument("Must provide a non-empty WorkspaceGroup");
  }

  if (!groupIsAllMatrixWorkspaces(wsGroup)) {
    throw std::invalid_argument(
        "Input WorkspaceGroup must only contain MatrixWorkspaces");
  }

  if (!groupContentsHaveSameX(wsGroup, spectrum)) {
    throw std::invalid_argument(
        "Input WorkspaceGroup must have same X data for all workspaces");
  }
}
