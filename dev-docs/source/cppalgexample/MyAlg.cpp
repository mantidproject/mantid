// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/MyAlg.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {
using Mantid::API::InstrumentValidator;
using Mantid::API::MatrixWorkspace;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::CompositeValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::StringListValidator;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MyAlg)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string MyAlg::name() const { return "MyAlg"; }

/// Algorithm's version for identification. @see Algorithm::version
int MyAlg::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MyAlg::category() const { return "Examples"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MyAlg::summary() const { return "Multiplies a workspace by a constant. There are some modes"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MyAlg::init() {
  auto instrumentValidator = std::make_shared<CompositeValidator>();
  instrumentValidator->add<InstrumentValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input, instrumentValidator),
      "An input workspace.");
  declareProperty("NumberToApply", EMPTY_DBL(), "Value to apply to workspace. This is extra information");

  std::vector<std::string> propOptions{"X", "Y"};
  declareProperty("WayToApply", propOptions.back(), std::make_shared<StringListValidator>(propOptions),
                  "Which axis to apply values to");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

std::map<std::string, std::string> MyAlg::validateInputs() {
  std::map<std::string, std::string> issues;

  if (!isDefault("NumberToApply")) {
    const double value = getProperty("NumberToApply");
    if (value == 42.) {
      issues["NumberToApply"] = "We do cannot answer that question";
    }
  }

  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MyAlg::exec() {
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  const double number = getProperty("NumberToApply");
  const std::string axisToApply = getPropertyValue("WayToApply");

  // set up the output
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
  }

  // set up a lambda for doing the transformation
  auto transformer = std::function<double(double)>([number](const double &value) { return value * number; });

  // do the actual work
  const auto NUM_HIST = outputWS->getNumberHistograms();
  if (axisToApply == "X") {
    for (std::size_t i = 0; i < NUM_HIST; ++i) {
      // this gets read/write axis to the x-values
      auto &axis = outputWS->getSpectrum(i).dataX();
      std::transform(axis.cbegin(), axis.cend(), axis.begin(), transformer);
    }
  } else if (axisToApply == "Y") {
    for (std::size_t i = 0; i < NUM_HIST; ++i) {
      // this gets read/write axis to the y-values
      auto &axis = outputWS->getSpectrum(i).dataY();
      std::transform(axis.cbegin(), axis.cend(), axis.begin(), transformer);
    }
  } else {
    std::stringstream msg;
    msg << "The developer forgot to write code for NumberToApply=" << axisToApply;
    throw std::runtime_error(msg.str());
  }

  // pass back the output workspace
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
