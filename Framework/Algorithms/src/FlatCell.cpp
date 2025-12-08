// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FlatCell.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/HistogramValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <limits>
#include <sstream>

using Mantid::HistogramData::BinEdges;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FlatCell)

using namespace Kernel;
using namespace API;
using namespace Mantid::DataObjects;

void FlatCell::init() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The name of the input Workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the Workspace containing the flat cell bins.");
}

/** Computes the mean of the input vector
 */
double mean(const std::vector<double> &values) {
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

/** Computes the standard deviation of the input vector
 */
double stddev(const std::vector<double> &values) {
  double m = mean(values);
  double accum = 0.0;
  for (double x : values) {
    accum += (x - m) * (x - m);
  }
  return std::sqrt(accum / static_cast<double>(values.size()));
}

/** Execution code.
 *  @throw std::invalid_argument If XMax is less than XMin
 */
void FlatCell::exec() {

  // Get the Input Workspace and the X values
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Verify that the Workspace contains just one spectrum and the correct number of values
  // Will be added when the test are written

  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
    setProperty("OutputWorkspace", outputWS);
  }

  // Get the X values
  const auto &Y = inputWS->readY(0);

  // Save the Low and High Angle Bank values into vectors
  std::vector<double> LAB(Y.begin() + 2, Y.begin() + 16385);
  std::vector<double> HAB(Y.begin() + 16386, Y.begin() + 17785);

  // Calculate the mean and stddev of the Original Data
  double meanLAB = mean(LAB);
  double stdLAB = stddev(LAB);

  double meanHAB = mean(HAB);
  double stdHAB = stddev(HAB);

  // Normalize the values in the LAB and HAB vectors
  for (auto &v : LAB) {
    v /= meanLAB;
  }
  for (auto &v : HAB) {
    v /= meanHAB;
  }

  // Create the array to save the output
  std::vector<double> out(17992, 0.0);
  std::copy(LAB.begin(), LAB.end(), out.begin() + 2);
  std::copy(HAB.begin(), HAB.end(), out.begin() + 16386);

  std::vector<double> HAB1(out.begin() + 16386, out.begin() + 16733);
  std::vector<double> HAB2(out.begin() + 16736, out.begin() + 17083);
  std::vector<double> HAB3(out.begin() + 17086, out.begin() + 17433);
  std::vector<double> HAB4(out.begin() + 17436, out.begin() + 17783);

  // Calculate the rescale factor of each high angle bank
  double rescaleHAB1 = 1.0 / mean(HAB1);
  double rescaleHAB2 = 1.0 / mean(HAB2);
  double rescaleHAB3 = 1.0 / mean(HAB3);
  double rescaleHAB4 = 1.0 / mean(HAB4);

  // Rescale the values in the HAB vectors
  for (auto &v : HAB1) {
    v *= rescaleHAB1;
  }
  for (auto &v : HAB2) {
    v *= rescaleHAB2;
  }
  for (auto &v : HAB3) {
    v *= rescaleHAB3;
  }
  for (auto &v : HAB4) {
    v *= rescaleHAB4;
  }

  // Copy the values in the output spectrum
  std::copy(HAB1.begin(), HAB1.end(), out.begin() + 16386);
  std::copy(HAB2.begin(), HAB2.end(), out.begin() + 16736);
  std::copy(HAB3.begin(), HAB3.end(), out.begin() + 17086);
  std::copy(HAB4.begin(), HAB4.end(), out.begin() + 17436);

  // Save the data into the output WS
  auto &O = outputWS->mutableY(0);
  std::copy(out.begin(), out.end(), O.begin());

  // Log for debugging
  g_log.warning() << "Normalized HAB1 mean = " << rescaleHAB1 << "\n";

  g_log.warning() << "Normalized HAB2 mean = " << rescaleHAB2 << "\n";

  g_log.warning() << "Normalized HAB3 mean = " << rescaleHAB3 << "\n";

  g_log.warning() << "Normalized HAB4 mean = " << rescaleHAB4 << "\n";
}

/** Execution code for EventWorkspaces
 */
void FlatCell::execEvent() {
  // To Do
}

} // namespace Mantid::Algorithms
