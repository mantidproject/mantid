#include "MantidGroupPlotGenerator.h"

using Mantid::API::WorkspaceGroup_const_sptr;
using Mantid::API::WorkspaceGroup_sptr;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::ExperimentInfo;

/**
 * Constructor
 * @param mantidUI :: [input] Pointer to the Mantid UI
 */
MantidGroupPlotGenerator::MantidGroupPlotGenerator(MantidUI *mantidUI)
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
    QString xLabelQ;
    auto matrixWS = createWorkspaceForGroupPlot(wsGroup, options, &xLabelQ);

    // Convert to correct X axis format
    convertXData(matrixWS, graphType);

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
      MultiLayer *plot = matrixToPlot->plotGraph2D(Graph::ColorMapContour);
      plot->activeGraph()->setXAxisTitle(xLabelQ);
      plot->activeGraph()->setYAxisTitle(options.axisName);
      plot->activeGraph()->setTitle(QString("Contour ").append(title));
    }
  }
}

/**
 * Create a workspace for the surface/contour plot from the given workspace
 * group. Returns title for the X axis.
 *
 * Note that only MatrixWorkspaces can be plotted, so if the group contains
 * Table or Peaks workspaces then it cannot be used.
 *
 * @param wsGroup :: [input] Pointer to workspace group to use as input
 * @param options :: [input] User input from dialog
 * @param xAxisTitle :: [output] Title for the X axis, read from the workspaces
 * in the group
 * @returns Pointer to the created workspace
 */
const MatrixWorkspace_sptr
MantidGroupPlotGenerator::createWorkspaceForGroupPlot(
    WorkspaceGroup_const_sptr wsGroup,
    const MantidSurfacePlotDialog::UserInputSurface &options,
    QString *xAxisTitle) const {
  MatrixWorkspace_sptr matrixWS;     // Workspace to return
  int index = options.plotIndex;     // which spectrum to plot from each WS
  QString logName = options.logName; // Log to read from for axis of XYZ plot
  if (wsGroup && groupIsAllMatrixWorkspaces(wsGroup)) {
    // Create workspace to hold the data
    // Each "spectrum" will be the data from one workspace
    int nWorkspaces = wsGroup->getNumberOfEntries();
    if (nWorkspaces > 0) {
      const auto firstWS =
          boost::dynamic_pointer_cast<const MatrixWorkspace>(wsGroup->getItem(
              0)); // Already checked group contains only MatrixWorkspaces
      std::string xAxisLabel, xAxisUnits;
      matrixWS = Mantid::API::WorkspaceFactory::Instance().create(
          firstWS, nWorkspaces, firstWS->blocksize(), firstWS->blocksize());
      matrixWS->setYUnitLabel(firstWS->YUnitLabel());
      xAxisLabel = firstWS->getXDimension()->getName();
      xAxisUnits = firstWS->getXDimension()->getUnits();


      // For each workspace in group, add data and log values
      std::vector<double> logValues;
      for (int i = 0; i < nWorkspaces; i++) {
        const auto ws = boost::dynamic_pointer_cast<const MatrixWorkspace>(
            wsGroup->getItem(i));
        if (ws) {
          auto X = ws->readX(index);
          auto Y = ws->readY(index);
          matrixWS->dataX(i).swap(X);
          matrixWS->dataY(i).swap(Y);
          if (logName == MantidSurfacePlotDialog::CUSTOM) {
            logValues.push_back(getSingleLogValue(i, options.customLogValues));
          } else {
            logValues.push_back(getSingleLogValue(i, ws, logName));
          }
        }
      }

      // Set log axis values by replacing the "spectra" axis
      matrixWS->replaceAxis(1, new Mantid::API::NumericAxis(logValues));

      // Generate title for the X axis
      *xAxisTitle = xAxisLabel.empty() ? "X" : xAxisLabel.c_str();
      if (!xAxisUnits.empty()) {
        xAxisTitle->append(" (").append(xAxisUnits.c_str()).append(")");
      }
    }
  }
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
 * Converts X data to correct (point/histo) format for the graph type.
 * Contour: convert points to histo, if not already.
 * Surface: convert histo to points, if not already.
 * Converts data in place.
 * @param ws :: [input, output] Workspace to change
 * @param graphType :: [input] Type of graph to be plotted
 */
void MantidGroupPlotGenerator::convertXData(MatrixWorkspace_sptr ws,
                                            Type graphType) const {
  if (graphType == Type::Contour) {
    convertPointsToHisto(ws);
  } else if (graphType == Type::Surface) {
    convertHistoToPoints(ws);
  }
}

/**
 * Utility method to convert a histogram workspace to points data
 * (centred bins) for surface plots.
 * @param ws :: [input, output] Workspace to convert
 */
void MantidGroupPlotGenerator::convertHistoToPoints(
    MatrixWorkspace_sptr ws) const {
  if (ws && ws->isHistogramData()) {
    auto alg = m_mantidUI->createAlgorithm("ConvertToPointData");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", "__NotUsed");
    alg->execute();
    ws = alg->getProperty("OutputWorkspace");
  }
}

/**
 * Utility method to convert a points workspace to histogram data
 * for contour plots.
 * @param ws :: [input, output] Workspace to convert
 */
void MantidGroupPlotGenerator::convertPointsToHisto(
    MatrixWorkspace_sptr ws) const {
  if (ws && !ws->isHistogramData()) {
    auto alg = m_mantidUI->createAlgorithm("ConvertToHistogram");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", "__NotUsed");
    alg->execute();
    ws = alg->getProperty("OutputWorkspace");
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
