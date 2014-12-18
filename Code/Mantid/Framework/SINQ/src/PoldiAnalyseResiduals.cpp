#include "MantidSINQ/PoldiAnalyseResiduals.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidSINQ/PoldiUtilities/PoldiResidualCorrelationCore.h"
#include "MantidSINQ/PoldiUtilities/PoldiDeadWireDecorator.h"

#include <numeric>

namespace Mantid {
namespace Poldi {

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiAnalyseResiduals)

//----------------------------------------------------------------------------------------------
/** Constructor
   */
PoldiAnalyseResiduals::PoldiAnalyseResiduals() : Algorithm() {}

//----------------------------------------------------------------------------------------------
/** Destructor
   */
PoldiAnalyseResiduals::~PoldiAnalyseResiduals() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PoldiAnalyseResiduals::name() const {
  return "PoldiAnalyseResiduals";
}

/// Algorithm's version for identification. @see Algorithm::version
int PoldiAnalyseResiduals::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiAnalyseResiduals::category() const {
  return "SINQ\\Poldi";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PoldiAnalyseResiduals::summary() const {
  return "Analysis of residuals after fitting POLDI 2D-spectra.";
}

/// Sums the counts of all spectra specified by the supplied list of workspace
/// indices.
double PoldiAnalyseResiduals::sumCounts(
    const DataObjects::Workspace2D_sptr &workspace,
    const std::vector<int> &workspaceIndices) const {
  double sum = 0.0;
  for (size_t i = 0; i < workspaceIndices.size(); ++i) {
    const MantidVec &counts = workspace->readY(workspaceIndices[i]);
    sum += std::accumulate(counts.begin(), counts.end(), 0.0);
  }

  return sum;
}

/// Counts the number of values in each spectrum specified by the list of
/// workspace indices.
size_t PoldiAnalyseResiduals::numberOfPoints(
    const DataObjects::Workspace2D_sptr &workspace,
    const std::vector<int> &workspaceIndices) const {
  size_t sum = 0;
  for (size_t i = 0; i < workspaceIndices.size(); ++i) {
    const MantidVec &counts = workspace->readY(workspaceIndices[i]);
    sum += counts.size();
  }

  return sum;
}

/// Adds the specified value to all spectra specified by the given workspace
/// indices.
void PoldiAnalyseResiduals::addValue(
    DataObjects::Workspace2D_sptr &workspace, double value,
    const std::vector<int> &workspaceIndices) const {
  for (size_t i = 0; i < workspaceIndices.size(); ++i) {
    MantidVec &counts = workspace->dataY(workspaceIndices[i]);
    for (size_t j = 0; j < counts.size(); ++j) {
      counts[j] += value;
    }
  }
}

/// Initialize algorithm
void PoldiAnalyseResiduals::init() {
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "MeasuredCountData", "", Direction::Input),
                  "Input workspace containing the measured data.");
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "FittedCountData", "", Direction::Input),
                  "Input workspace containing the fitted data.");
  declareProperty("LambdaMin", 1.1, "Minimum wavelength to be considered.");
  declareProperty("LambdaMax", 5.0, "Maximum wavelength to be considered.");
  declareProperty("MaxIterations", 0, "Maximum number of iterations. Default 0 "
                                      "does not limit number of iterations.");
  declareProperty(
      "MaxRelativeChange", 1.0,
      "Relative change in counts (in percent) that should be reached.");

  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "OutputWorkspace", "", Direction::Output),
                  "Workspace containing the residual correlation spectrum.");
}

/// Create workspace that contains the residuals.
DataObjects::Workspace2D_sptr PoldiAnalyseResiduals::calculateResidualWorkspace(
    const DataObjects::Workspace2D_sptr &measured,
    const DataObjects::Workspace2D_sptr &calculated) {
  IAlgorithm_sptr minus = createChildAlgorithm("Minus");
  minus->setProperty("LHSWorkspace", measured);
  minus->setProperty("RHSWorkspace", calculated);
  minus->execute();

  MatrixWorkspace_sptr fg = minus->getProperty("OutputWorkspace");
  DataObjects::Workspace2D_sptr residuals =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(fg);

  return residuals;
}

/// Normalize residuals by subtracting the average value from each cell, so that
/// the sum is 0 afterwards.
void PoldiAnalyseResiduals::normalizeResiduals(
    DataObjects::Workspace2D_sptr &residuals,
    const std::vector<int> &validWorkspaceIndices) {
  double sumOfResiduals = sumCounts(residuals, validWorkspaceIndices);
  double dataPointCount =
      static_cast<double>(numberOfPoints(residuals, validWorkspaceIndices));

  addValue(residuals, -sumOfResiduals / dataPointCount, validWorkspaceIndices);
}

/// Add workspaces and return result.
DataObjects::Workspace2D_sptr
PoldiAnalyseResiduals::addWorkspaces(const DataObjects::Workspace2D_sptr &lhs,
                                     const DataObjects::Workspace2D_sptr &rhs) {
  IAlgorithm_sptr plus = createChildAlgorithm("Plus");
  plus->setProperty("LHSWorkspace", lhs);
  plus->setProperty("RHSWorkspace", rhs);
  plus->execute();

  MatrixWorkspace_sptr plusResult = plus->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<DataObjects::Workspace2D>(plusResult);
}

/// Output iteration information to log, report progress.
void PoldiAnalyseResiduals::logIteration(int iteration, double relativeChange) {
  g_log.information() << "Iteration " << iteration
                      << ", change=" << relativeChange << "%" << std::endl;

  int maxIterations = getProperty("MaxIterations");
  if (maxIterations > 0) {
    progress(static_cast<double>(iteration) /
             static_cast<double>(maxIterations));
  }
}

/// Checks if next iteration is allowed based on number of iterations so far and
/// last relative change.
bool PoldiAnalyseResiduals::nextIterationAllowed(int iterations,
                                                 double relativeChange) {
  return relativeChangeIsLargerThanLimit(relativeChange) &&
         !iterationLimitReached(iterations);
}

/// Returns true if the number is larger than the MaxRelativeChange-property
bool
PoldiAnalyseResiduals::relativeChangeIsLargerThanLimit(double relativeChange) {
  double maxRelativeChange = getProperty("MaxRelativeChange");

  return relativeChange > maxRelativeChange;
}

/// Returns true when the iteration limit is reached or false if there is no
/// iteration limit.
bool PoldiAnalyseResiduals::iterationLimitReached(int iterations) {
  int maxIterations = getProperty("MaxIterations");

  if (maxIterations > 0) {
    return iterations >= maxIterations;
  }

  return false;
}

/// Calculate the relative change in residuals with respect to the supplied
/// total number of counts
double PoldiAnalyseResiduals::relativeCountChange(
    const DataObjects::Workspace2D_sptr &sum, double totalMeasuredCounts) {
  const MantidVec &corrCounts = sum->readY(0);
  double csum = 0.0;
  for (auto it = corrCounts.begin(); it != corrCounts.end(); ++it) {
    csum += fabs(*it);
  }

  return csum / totalMeasuredCounts * 100.0;
}

void PoldiAnalyseResiduals::exec() {
  DataObjects::Workspace2D_sptr measured = getProperty("MeasuredCountData");
  DataObjects::Workspace2D_sptr calculated = getProperty("FittedCountData");

  PoldiInstrumentAdapter_sptr poldiInstrument =
      boost::make_shared<PoldiInstrumentAdapter>(measured);
  // Dead wires need to be taken into account
  PoldiAbstractDetector_sptr deadWireDetector =
      boost::make_shared<PoldiDeadWireDecorator>(measured->getInstrument(),
                                                 poldiInstrument->detector());

  // Since the valid workspace indices are required for some calculations, we
  // extract and keep them
  const std::vector<int> &validWorkspaceIndices =
      deadWireDetector->availableElements();

  // Subtract calculated from measured to get residuals
  DataObjects::Workspace2D_sptr residuals =
      calculateResidualWorkspace(measured, calculated);

  // Normalize residuals so that they are 0.
  normalizeResiduals(residuals, validWorkspaceIndices);

  // Residual correlation core which will be used iteratively.
  PoldiResidualCorrelationCore core(g_log, 0.1);
  core.setInstrument(deadWireDetector, poldiInstrument->chopper());

  double lambdaMin = getProperty("LambdaMin");
  double lambdaMax = getProperty("LambdaMax");
  core.setWavelengthRange(lambdaMin, lambdaMax);

  // One iteration is always necessary
  DataObjects::Workspace2D_sptr sum = core.calculate(residuals, calculated);

  // For keeping track of the relative changes the sum of measured counts is
  // required
  double sumOfMeasuredCounts = sumCounts(measured, validWorkspaceIndices);
  double relativeChange = relativeCountChange(sum, sumOfMeasuredCounts);

  int iteration = 1;

  logIteration(iteration, relativeChange);

  // Iterate until conditions are met, accumulate correlation spectra in sum.
  while (nextIterationAllowed(iteration, relativeChange)) {
    ++iteration;

    DataObjects::Workspace2D_sptr corr = core.calculate(residuals, calculated);
    relativeChange = relativeCountChange(corr, sumOfMeasuredCounts);

    sum = addWorkspaces(sum, corr);

    logIteration(iteration, relativeChange);
  }

  g_log.notice() << "Finished after " << iteration
                 << " iterations, final change=" << relativeChange << std::endl;

  // Return final correlation spectrum.
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(sum));
}

} // namespace SINQ
} // namespace Mantid
