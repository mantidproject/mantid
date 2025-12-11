// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FlatCell.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/HistogramValidator.h"

#include <limits>
#include <sstream>

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
double FlatCell::mean(const std::vector<double> &values) {
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

/** Computes the standard deviation of the input vector
 */
double FlatCell::stddev(const std::vector<double> &values) {
  double m = mean(values);
  double accum = std::accumulate(values.begin(), values.end(), 0.0,
                                 [m](double total, double x) { return total + (x - m) * (x - m); });
  return std::sqrt(accum / static_cast<double>(values.size()));
}

/** Scales each of the elements of the input vector
 */
void FlatCell::scale(std::vector<double> &values, double factor) {
  std::transform(values.begin(), values.end(), values.begin(), [factor](double x) { return x * factor; });
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
  std::vector<double> LAB(Y.begin() + 2, Y.begin() + 16386);

  std::vector<double> HAB(Y.begin() + 16386, Y.begin() + 17786);

  // Calculate the mean and stddev of the Original Data
  double meanLAB = FlatCell::mean(LAB);
  // double stdLAB = stddev(LAB);

  double meanHAB = FlatCell::mean(HAB);
  // double stdHAB = stddev(HAB);

  // Normalize the values in the LAB and HAB vectors
  FlatCell::scale(LAB, 1 / meanLAB);
  FlatCell::scale(HAB, 1 / meanHAB);

  // Create the array to save the output
  std::vector<double> out(17992, 0.0);
  std::copy(LAB.begin(), LAB.end(), out.begin() + 2);
  std::copy(HAB.begin(), HAB.end(), out.begin() + 16386);

  std::vector<double> HAB1(out.begin() + 16386, out.begin() + 16734);
  std::vector<double> HAB2(out.begin() + 16736, out.begin() + 17084);
  std::vector<double> HAB3(out.begin() + 17086, out.begin() + 17434);
  std::vector<double> HAB4(out.begin() + 17436, out.begin() + 17784);

  // Calculate the rescale factor of each high angle bank
  double rescaleHAB1 = 1.0 / FlatCell::mean(HAB1);
  double rescaleHAB2 = 1.0 / FlatCell::mean(HAB2);
  double rescaleHAB3 = 1.0 / FlatCell::mean(HAB3);
  double rescaleHAB4 = 1.0 / FlatCell::mean(HAB4);

  // Rescale the values in the HAB vectors
  FlatCell::scale(HAB1, rescaleHAB1);
  FlatCell::scale(HAB2, rescaleHAB2);
  FlatCell::scale(HAB3, rescaleHAB3);
  FlatCell::scale(HAB4, rescaleHAB4);

  // Copy the values in the output spectrum
  std::copy(HAB1.begin(), HAB1.end(), out.begin() + 16386);
  std::copy(HAB2.begin(), HAB2.end(), out.begin() + 16736);
  std::copy(HAB3.begin(), HAB3.end(), out.begin() + 17086);
  std::copy(HAB4.begin(), HAB4.end(), out.begin() + 17436);

  // Manually change the values of the edge cases
  out[16735] = out[16735] * rescaleHAB2;
  out[17435] = out[17435] * rescaleHAB4;
  out[17784] = out[17784] * rescaleHAB4;

  // Save the Y data into the output WS
  auto &O = outputWS->mutableY(0);
  std::copy(out.begin(), out.end(), O.begin());

  // Update the E values
  std::vector<double> e(17992, 0.0);
  auto &E = outputWS->mutableE(0);
  std::copy(e.begin(), e.end(), E.begin());
}

/** Execution code for EventWorkspaces
 */
void FlatCell::execEvent() {
  // To Do
}

} // namespace Mantid::Algorithms
