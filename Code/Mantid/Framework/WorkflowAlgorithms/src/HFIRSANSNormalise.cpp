//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/HFIRSANSNormalise.h"
#include <boost/algorithm/string.hpp>
#include "Poco/NumberFormatter.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace WorkflowAlgorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HFIRSANSNormalise)

void HFIRSANSNormalise::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Workspace to be corrected");

  std::vector<std::string> normOptions;
  normOptions.push_back("Monitor");
  normOptions.push_back("Timer");
  this->declareProperty("NormalisationType", "Monitor",
                        boost::make_shared<StringListValidator>(normOptions),
                        "Type of Normalisation to use");

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Corrected workspace");
  declareProperty("OutputMessage", "", Direction::Output);
}

void HFIRSANSNormalise::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  std::string normalisation = getProperty("NormalisationType");

  // Get the monitor or timer
  boost::algorithm::to_lower(normalisation);
  Mantid::Kernel::Property *prop = inputWS->run().getProperty(normalisation);
  Mantid::Kernel::PropertyWithValue<double> *dp =
      dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
  double norm_count = *dp;

  double factor;
  if (boost::iequals(normalisation, "monitor")) {
    factor = 1.0e8 / norm_count;
  } else {
    factor = 1.0 / norm_count;
  }

  IAlgorithm_sptr scaleAlg = createChildAlgorithm("Scale");
  scaleAlg->setProperty("InputWorkspace", inputWS);
  scaleAlg->setProperty("OutputWorkspace", outputWS);
  scaleAlg->setProperty("Factor", factor);
  scaleAlg->setProperty("Operation", "Multiply");
  scaleAlg->executeAsChildAlg();
  MatrixWorkspace_sptr scaledWS = scaleAlg->getProperty("OutputWorkspace");

  setProperty("OutputWorkspace", scaledWS);
  setProperty("OutputMessage", "Normalisation by " + normalisation + ": " +
                                   Poco::NumberFormatter::format(norm_count));
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
