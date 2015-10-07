//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/NormaliseToUnity.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(NormaliseToUnity)

using namespace Kernel;
using namespace API;

/** Initialisation method.
 *
 */
void NormaliseToUnity::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<CommonBinsValidator>();

  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator),
                  "The name of the input workspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name with which to store the output workspace in the [[Analysis "
      "Data Service]]");

  declareProperty("RangeLower", EMPTY_DBL(),
                  "The X (frame) value to integrate from");
  declareProperty("RangeUpper", EMPTY_DBL(),
                  "The X (frame) value to integrate to");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "StartWorkspaceIndex", 0, mustBePositive,
      "The lowest workspace index of the specta that will be integrated");
  // As the property takes ownership of the validator pointer, have to take care
  // to pass in a unique
  // pointer to each property.
  declareProperty(
      "EndWorkspaceIndex", EMPTY_INT(), mustBePositive,
      "The highest workspace index of the spectra that will be integrated");
  declareProperty("IncludePartialBins", false,
                  "If true then partial bins from the beginning and end of the "
                  "input range are also included in the integration.");
  declareProperty(
      "IncludeMonitors", true,
      "Whether to include monitor spectra in the sum (default: yes)");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void NormaliseToUnity::exec() {
  // Try and retrieve the optional properties
  double m_MinRange = getProperty("RangeLower");
  double m_MaxRange = getProperty("RangeUpper");
  int m_MinSpec = getProperty("StartWorkspaceIndex");
  int m_MaxSpec = getProperty("EndWorkspaceIndex");
  const bool keepMonitors = getProperty("IncludeMonitors");
  const bool incPartBins = getProperty("IncludePartialBins");

  Progress progress(this, 0.0, 1.0, 3);

  // Get the input workspace
  MatrixWorkspace_sptr localworkspace = getProperty("InputWorkspace");

  // Sum up all the wavelength bins
  IAlgorithm_sptr integrateAlg = createChildAlgorithm("Integration");
  integrateAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                                  localworkspace);
  integrateAlg->setProperty<double>("RangeLower", m_MinRange);
  integrateAlg->setProperty<double>("RangeUpper", m_MaxRange);
  integrateAlg->setProperty<int>("StartWorkspaceIndex", m_MinSpec);
  integrateAlg->setProperty<int>("EndWorkspaceIndex", m_MaxSpec);
  integrateAlg->setProperty<bool>("IncludePartialBins", incPartBins);
  integrateAlg->executeAsChildAlg();
  progress.report("Normalising to unity");

  MatrixWorkspace_sptr integrated =
      integrateAlg->getProperty("OutputWorkspace");

  // Sum all the spectra of the integrated workspace
  IAlgorithm_sptr sumAlg = createChildAlgorithm("SumSpectra");
  sumAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", integrated);
  sumAlg->setProperty<bool>("IncludeMonitors", keepMonitors);
  sumAlg->executeAsChildAlg();
  progress.report("Normalising to unity");

  MatrixWorkspace_sptr summed = sumAlg->getProperty("OutputWorkspace");

  // Divide by the sum
  MatrixWorkspace_sptr result = localworkspace / summed;
  progress.report("Normalising to unity");

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", result);

  return;
}

} // namespace Algorithms
} // namespace Mantid
