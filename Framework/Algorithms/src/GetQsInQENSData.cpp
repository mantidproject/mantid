// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GetQsInQENSData.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitConversion.h"

#include <stdexcept>

namespace {
Mantid::Kernel::Logger g_log("ConvolutionFitSequential");
}

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;
using namespace Geometry;

// Register the Algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GetQsInQENSData)

// Initializes the Algorithm
void GetQsInQENSData::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Input QENS data as MatrixWorkspace");

  declareProperty("RaiseMode", false,
                  "Set to True if an Exception, instead of "
                  "any empty list of Q values, is "
                  "desired.");

  declareProperty(
      std::make_unique<ArrayProperty<double>>("Qvalues", Direction::Output));
}

/*
 * Validates the input properties
 */
std::map<std::string, std::string> GetQsInQENSData::validateInputs() {
  std::map<std::string, std::string> issues;
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  // Check whether the input workspace could be found
  if (!inputWs) {
    issues["InputWorkspace"] = "InputWorkspace is not a MatrixWorkspace";
  }

  return issues;
}

/*
 * Executes the Algorithm
 */
void GetQsInQENSData::exec() {
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  try {
    setProperty("Qvalues", extractQValues(inputWs));
  } catch (std::exception &e) {
    g_log.error(e.what());

    // If the RaiseMode property has been set to true, raise any
    // exception which is thrown.
    bool inRaiseMode = getProperty("RaiseMode");
    if (inRaiseMode) {
      throw;
    }

    setProperty("Qvalues", std::vector<double>());
  }
}

/*
 * Extracts Q-values from the specified workspace.
 *
 * @param workspace The workspace from which to extract Q-values.
 * @return          The extracted Q-values as a vector.
 */
MantidVec GetQsInQENSData::extractQValues(
    const Mantid::API::MatrixWorkspace_sptr workspace) {
  size_t numSpectra = workspace->getNumberHistograms();
  Axis *qAxis;

  try {
    qAxis = workspace->getAxis(1);
  } catch (std::exception &) {
    throw std::runtime_error("Vertical axis is empty");
  }

  MantidVec qValues(qAxis->length());

  // Check if the specified workspace is already in Q-space.
  if (qAxis->unit()->unitID() == "MomentumTransfer") {

    // Add axis values to vector of Q-values
    for (size_t i = 0; i < qAxis->length(); i++) {
      qValues[i] = qAxis->getValue(i);
    }

    // Check if the Q-values are stored as histogram data.
    if (qValues.size() == numSpectra + 1) {
      // Convert Q-values to point values.
      qValues.pop_back();
      qValues.erase(qValues.begin());
      std::transform(qValues.begin(), qValues.end(), qValues.begin(),
                     std::bind2nd(std::divides<double>(), 2.0));
    }
  } else {

    // Iterate over all spectrum in the specified workspace.
    try {

      for (size_t i = 0; i < numSpectra; i++) {
        IDetector_const_sptr detector = workspace->getDetector(i);
        double efixed = workspace->getEFixed(detector);
        double theta = 0.5 * workspace->detectorTwoTheta(*detector);
        qValues[i] = UnitConversion::convertToElasticQ(theta, efixed);
      }
    } catch (std::exception &) {
      throw std::runtime_error(
          "Detectors are missing from the input workspace");
    }
  }

  return qValues;
}
} // namespace Algorithms
} // namespace Mantid
