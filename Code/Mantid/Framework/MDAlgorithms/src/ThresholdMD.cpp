#include "MantidMDAlgorithms/ThresholdMD.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/Progress.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ThresholdMD)

std::string LessThan() { return "Less Than"; }

std::string GreaterThan() { return "Greater Than"; }

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ThresholdMD::ThresholdMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ThresholdMD::~ThresholdMD() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ThresholdMD::name() const { return "ThresholdMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int ThresholdMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ThresholdMD::category() const { return "MDAlgorithms"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ThresholdMD::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input workspace.");

  std::vector<std::string> propOptions;
  propOptions.push_back(LessThan());
  propOptions.push_back(GreaterThan());

  declareProperty("Condition", LessThan(),
                  boost::make_shared<StringListValidator>(propOptions),
                  "Selected threshold condition. Any value which does meet "
                  "this condition with respect to the ReferenceValue will be "
                  "overwritten.");

  declareProperty("ReferenceValue", 0.0,
                  "Comparator value used by the Condition.");

  declareProperty("OverwriteWithZero", true, "Flag for enabling overwriting "
                                             "with a custom value. Defaults to "
                                             "overwrite signals with zeros.");

  declareProperty("CustomOverwriteValue", 0.0,
                  "Custom overwrite value for the signal. Defaults to zero.");
  setPropertySettings(
      "CustomOverwriteValue",
      new EnabledWhenProperty("OverwriteWithZero", IS_NOT_DEFAULT));

  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output thresholded workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ThresholdMD::exec() {
  IMDHistoWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string condition = getProperty("Condition");
  const double referenceValue = getProperty("ReferenceValue");
  const bool doOverwriteWithZero = getProperty("OverwriteWithZero");
  double customOverwriteValue = getProperty("CustomOverwriteValue");
  if (doOverwriteWithZero) {
    customOverwriteValue = 0;
  }

  IMDHistoWorkspace_sptr outWS = getProperty("OutputWorkspace");
  if (outWS != inputWS) {
    g_log.debug("Deep copy input workspace as output workspace.");
    IAlgorithm_sptr alg = createChildAlgorithm("CloneMDWorkspace");
    alg->setProperty("InputWorkspace", inputWS);
    alg->executeAsChildAlg();
    IMDWorkspace_sptr temp = alg->getProperty("OutputWorkspace");
    outWS = boost::dynamic_pointer_cast<IMDHistoWorkspace>(temp);
  }

  const int64_t nPoints = inputWS->getNPoints();

  boost::function<bool(double)> comparitor =
      boost::bind(std::less<double>(), _1, referenceValue);
  if (condition == GreaterThan()) {
    comparitor = boost::bind(std::greater<double>(), _1, referenceValue);
  }

  Progress prog(this, 0, 1, 100);
  int64_t frequency = nPoints;
  if (nPoints > 100) {
    frequency = nPoints / 100;
  }

  PARALLEL_FOR2(inputWS, outWS)
  for (int64_t i = 0; i < nPoints; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const double signalAt = inputWS->getSignalAt(i);
    if (comparitor(signalAt)) {
      outWS->setSignalAt(i, customOverwriteValue);
    }
    if (i % frequency == 0) {
      prog.report();
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", outWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
