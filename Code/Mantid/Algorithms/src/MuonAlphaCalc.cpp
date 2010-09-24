//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>

#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/MuonAlphaCalc.h"

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using API::Progress;

// Register the class into the algorithm factory
DECLARE_ALGORITHM( MuonAlphaCalc)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void MuonAlphaCalc::init()
{
  // reset the logger's name
  this->g_log.setName("Algorithms::MuonAlphaCalc");

  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "",
      Direction::Input), "Name of the input workspace");

  std::vector<int> forwardDefault;
  forwardDefault.push_back(1);
  declareProperty(new ArrayProperty<int> ("ForwardSpectra", forwardDefault, Direction::Input),
      "The spectra numbers of the forward group (default to 1)");

  std::vector<int> backwardDefault;
  backwardDefault.push_back(2);
  declareProperty(new ArrayProperty<int> ("BackwardSpectra", backwardDefault, Direction::Input),
      "The spectra numbers of the backward group (default to 2)");

  declareProperty("FirstGoodValue", EMPTY_DBL(), "First good value (default lowest value of x)", Direction::Input);
  declareProperty("LastGoodValue", EMPTY_DBL(), "Last good value (default highest value of x)", Direction::Input);

  declareProperty("Alpha", 1.0, "The alpha efficiency (default to 1.0)", Direction::Output);
}

/** Executes the algorithm
 *
 */
void MuonAlphaCalc::exec()
{
  std::vector<int> forwardSpectraList = getProperty("ForwardSpectra");
  std::vector<int> backwardSpectraList = getProperty("BackwardSpectra");

  double alpha = getProperty("Alpha"); 

  // no point in attempting to calculate alpha if input workspace on contains
  // one spectra
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  if ( inputWS->getNumberHistograms() < 2 )
  {
    g_log.error() << "Can't calculate alpha value for workspace which"
      << " contains one spectrum. A default value of alpha = 1.0 is returned";
    setProperty("Alpha", alpha);
    return;
  }

  // if for some reason the size of forward and backward lists are zero
  // default these to their defaults
  if ( forwardSpectraList.empty() )
    forwardSpectraList.push_back(1);
  if ( backwardSpectraList.empty() )
    backwardSpectraList.push_back(2);


  // first step is to create two workspaces which groups all forward and
  // backward spectra

  API::IAlgorithm_sptr groupForward = createSubAlgorithm("GroupDetectors");
  groupForward->setProperty("InputWorkspace", inputWS);
  groupForward->setProperty("OutputWorkspace", "tmp");
  groupForward->setProperty("SpectraList", forwardSpectraList);
  groupForward->setProperty("KeepUngroupedSpectra", false);
  groupForward->execute();
  API::MatrixWorkspace_sptr forwardWS = groupForward->getProperty("OutputWorkspace");

  API::IAlgorithm_sptr groupBackward = createSubAlgorithm("GroupDetectors");
  groupBackward->setProperty("InputWorkspace", inputWS);
  groupBackward->setProperty("OutputWorkspace", "tmp");
  groupBackward->setProperty("SpectraList", backwardSpectraList);
  groupBackward->setProperty("KeepUngroupedSpectra", false);
  groupBackward->execute();
  API::MatrixWorkspace_sptr backwardWS = groupBackward->getProperty("OutputWorkspace");


  // calculate sum of forward counts

  double firstGoodvalue = getProperty("FirstGoodValue");
  double lastGoodvalue = getProperty("LastGoodValue");

  API::IAlgorithm_sptr integr = createSubAlgorithm("Integration");
  integr->setProperty("InputWorkspace", forwardWS);
  integr->setPropertyValue("OutputWorkspace","tmp");
  if (firstGoodvalue != EMPTY_DBL())
    integr->setProperty("RangeLower", firstGoodvalue);
  if (lastGoodvalue != EMPTY_DBL())
    integr->setProperty("RangeUpper", lastGoodvalue);
  integr->execute();
  API::MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

  double sumForward = out->readY(0)[0];

  if ( sumForward < 0 )
  {
    g_log.error() << "Sum of forward detector counts is negative."
      << "Therefore can't calculate alpha. Return alpha = 1.0.";
    setProperty("Alpha", alpha);
    return;
  }


  // calculate sum of backward counts

  API::IAlgorithm_sptr integrB = createSubAlgorithm("Integration");
  integrB->setProperty("InputWorkspace", backwardWS);
  integrB->setPropertyValue("OutputWorkspace","tmp");
  if (firstGoodvalue != EMPTY_DBL())
    integrB->setProperty("RangeLower", firstGoodvalue);
  if (lastGoodvalue != EMPTY_DBL())
    integrB->setProperty("RangeUpper", lastGoodvalue);
  integrB->execute();
  out = integrB->getProperty("OutputWorkspace");

  double sumBackward = out->readY(0)[0];

  if ( sumBackward < 0 )
  {
    g_log.error() << "Sum of backward detector counts is negative or zero."
      << "Therefore can't calculate alpha. Return alpha = 1.0.";
    setProperty("Alpha", alpha);
    return;
  }

  // finally calculate alpha

  setProperty("Alpha", sumForward/sumBackward);
}

} // namespace Algorithm
} // namespace Mantid


