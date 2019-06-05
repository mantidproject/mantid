// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/HFIRSANSNormalise.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ListValidator.h"

#include <boost/algorithm/string.hpp>

#include "Poco/NumberFormatter.h"

namespace Mantid {
namespace WorkflowAlgorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HFIRSANSNormalise)

void HFIRSANSNormalise::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "Workspace to be corrected");

  std::vector<std::string> normOptions{"Monitor", "Timer"};
  this->declareProperty("NormalisationType", "Monitor",
                        boost::make_shared<StringListValidator>(normOptions),
                        "Type of Normalisation to use");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Corrected workspace");
  declareProperty("OutputMessage", "", Direction::Output);
}

void HFIRSANSNormalise::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  std::string normalisation = getProperty("NormalisationType");

  // Get the monitor or timer
  boost::algorithm::to_lower(normalisation);
  double norm_count =
      inputWS->run().getPropertyValueAsType<double>(normalisation);

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
