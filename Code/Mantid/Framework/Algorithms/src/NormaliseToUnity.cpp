/*WIKI* 

NormaliseToUnity uses [[Integration]] to sum up all the X bins, then sums up the resulting spectra using [[SumSpectra]]. Each bin of the input workspace is then divided by the total sum, regardless of whether a bin was included in the sum or not. It is thus possible to normalize a workspace so that a range of X bins and spectra sums to 1. In that case the sum of the whole workspace will likely not be equal to 1.



*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/NormaliseToUnity.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(NormaliseToUnity)

/// Sets documentation strings for this algorithm
void NormaliseToUnity::initDocs()
{
  this->setWikiSummary("NormaliseToUnity takes a 2D [[workspace]] or an [[EventWorkspace]] as input and normalises it to 1. Optionally, the range summed can be restricted in either dimension. ");
  this->setOptionalMessage("NormaliseToUnity takes a 2D workspace or an EventWorkspace as input and normalises it to 1. Optionally, the range summed can be restricted in either dimension.");
}


using namespace Kernel;
using namespace API;


/** Initialisation method.
 *
 */
void NormaliseToUnity::init()
{
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<CommonBinsValidator>();

  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, wsValidator));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("RangeLower",EMPTY_DBL());
  declareProperty("RangeUpper",EMPTY_DBL());
  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePositive);
  // As the property takes ownership of the validator pointer, have to take care to pass in a unique
  // pointer to each property.
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePositive);
  declareProperty("IncludePartialBins", false, "If true then partial bins from the beginning and end of the input range are also included in the integration.");
  declareProperty("IncludeMonitors",true,"Whether to include monitor spectra in the sum (default: yes)");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void NormaliseToUnity::exec()
{
  // Try and retrieve the optional properties
  double m_MinRange = getProperty("RangeLower");
  double m_MaxRange = getProperty("RangeUpper");
  int m_MinSpec = getProperty("StartWorkspaceIndex");
  int m_MaxSpec = getProperty("EndWorkspaceIndex");
  const bool keepMonitors = getProperty("IncludeMonitors");
  const bool incPartBins = getProperty("IncludePartialBins");

  Progress progress(this,0.0,1.0,3);

  // Get the input workspace
  MatrixWorkspace_sptr localworkspace = getProperty("InputWorkspace");

  // Sum up all the wavelength bins
  IAlgorithm_sptr integrateAlg = createSubAlgorithm("Integration");
  integrateAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", localworkspace);
  integrateAlg->setProperty<double>("RangeLower", m_MinRange);
  integrateAlg->setProperty<double>("RangeUpper", m_MaxRange);
  integrateAlg->setProperty<int>("StartWorkspaceIndex", m_MinSpec);
  integrateAlg->setProperty<int>("EndWorkspaceIndex", m_MaxSpec);
  integrateAlg->setProperty<bool>("IncludePartialBins", incPartBins);
  integrateAlg->executeAsSubAlg();
  progress.report("Normalising to unity");

  MatrixWorkspace_sptr integrated = integrateAlg->getProperty("OutputWorkspace");

  //Sum all the spectra of the integrated workspace
  IAlgorithm_sptr sumAlg = createSubAlgorithm("SumSpectra");
  sumAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", integrated);
  sumAlg->setProperty<bool>("IncludeMonitors", keepMonitors);
  sumAlg->executeAsSubAlg();
  progress.report("Normalising to unity");

  MatrixWorkspace_sptr summed = sumAlg->getProperty("OutputWorkspace");

  // Divide by the sum
  MatrixWorkspace_sptr result = localworkspace/summed;
  progress.report("Normalising to unity");

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", result);

  return;
}

} // namespace Algorithms
} // namespace Mantid
