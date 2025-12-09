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
  std::vector<double> LAB(Y.begin() + 2, Y.begin() + 16386);

  // Log for debugging
  // g_log.warning() << "Size Original LAB = " << LAB.size() << "\n";
  // g_log.warning() << "Original LAB[0] = " << LAB[0] << "\n";
  // g_log.warning() << "Original LAB[-1] = " << LAB[LAB.size() - 1] << "\n";

  std::vector<double> HAB(Y.begin() + 16386, Y.begin() + 17786);

  // Log for debugging
  // g_log.warning() << "Size Original HAB = " << HAB.size() << "\n";
  // g_log.warning() << "Original HAB[0] = " << HAB[0] << "\n";
  // g_log.warning() << "Original HAB[-1] = " << HAB[HAB.size() - 1] << "\n";

  // Calculate the mean and stddev of the Original Data
  double meanLAB = mean(LAB);
  double stdLAB = stddev(LAB);

  // Log for debugging
  // g_log.warning() << "Mean LAB = " << meanLAB << "\n";
  // g_log.warning() << "Std Dev LAB = " <<  stdLAB << "\n";

  double meanHAB = mean(HAB);
  double stdHAB = stddev(HAB);

  // Log for debugging
  // g_log.warning() << "Mean HAB = " << meanHAB << "\n";
  g_log.warning() << "Std Dev HAB = " << stdHAB << "\n";

  // Normalize the values in the LAB and HAB vectors
  for (auto &v : LAB) {
    v /= meanLAB;
  }

  // Log for debugging
  // g_log.warning() << "Size Normalized LAB = " << LAB.size() << "\n";
  // g_log.warning() << "Normalized LAB[0] = " << LAB[0] << "\n";
  // g_log.warning() << "Normalized LAB[-1] = " << LAB[LAB.size() - 1] << "\n";

  for (auto &v : HAB) {
    v /= meanHAB;
  }

  // Log for debugging
  // g_log.warning() << "Size Normalized HAB = " << HAB.size() << "\n";
  // g_log.warning() << "Normalized HAB[0] = " << HAB[0] << "\n";
  // g_log.warning() << "Normalized HAB[-1] = " << HAB[HAB.size() - 1] << "\n";

  // Create the array to save the output
  std::vector<double> out(17992, 0.0);
  std::copy(LAB.begin(), LAB.end(), out.begin() + 2);
  std::copy(HAB.begin(), HAB.end(), out.begin() + 16386);

  // Log for debugging
  // g_log.warning() << "Size out = " << out.size() << "\n";
  // g_log.warning() << "out[2] = " << out[0+2] << "\n";
  // g_log.warning() << "out[17785] = " << out[17785] << "\n";

  std::vector<double> HAB1(out.begin() + 16386, out.begin() + 16734);
  std::vector<double> HAB2(out.begin() + 16736, out.begin() + 17084);
  std::vector<double> HAB3(out.begin() + 17086, out.begin() + 17434);
  std::vector<double> HAB4(out.begin() + 17436, out.begin() + 17784);

  // Log for debugging
  g_log.warning() << "HAB1 size = " << HAB1.size() << "\n";
  g_log.warning() << "HAB2 size = " << HAB2.size() << "\n";
  g_log.warning() << "HAB3 size = " << HAB3.size() << "\n";
  g_log.warning() << "HAB4 size = " << HAB4.size() << "\n";

  // Calculate the rescale factor of each high angle bank
  double rescaleHAB1 = 1.0 / mean(HAB1);
  double rescaleHAB2 = 1.0 / mean(HAB2);
  double rescaleHAB3 = 1.0 / mean(HAB3);
  double rescaleHAB4 = 1.0 / mean(HAB4);

  // Log for debugging
  // g_log.warning() << "Rescale HAB1 parameter = " << rescaleHAB1 << "\n";
  // g_log.warning() << "Rescale HAB2 parameter = " << rescaleHAB2 << "\n";
  // g_log.warning() << "Rescale HAB3 parameter = " << rescaleHAB3 << "\n";
  // g_log.warning() << "Rescale HAB4 parameter = " << rescaleHAB4 << "\n";

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

  // Manually change the values of the edge cases
  out[16735] = out[16735] * rescaleHAB2;
  out[17435] = out[17435] * rescaleHAB4;
  out[17784] = out[17784] * rescaleHAB4;
  // std::copy(HAB1.begin(), HAB1.end(), out.begin() + 16386);
  // std::copy(HAB2.begin(), HAB2.end(), out.begin() + 16736);
  // std::copy(HAB3.begin(), HAB3.end(), out.begin() + 17086);
  // std::copy(HAB4.begin(), HAB4.end(), out.begin() + 17436);

  // Log for debugging
  // g_log.warning() << "Size out = " << out.size() << "\n";
  // g_log.warning() << "HAB1 (LOWER) out[16387] = " << out[16386] << "\n";
  // g_log.warning() << "HAB1 (UPPER) out[16734] = " << out[16733] << "\n";
  // g_log.warning() << "HAB2 (LOWER) out[16737] = " << out[16736] << "\n";
  // g_log.warning() << "HAB2 (UPPER) out[17084] = " << out[17083] << "\n";
  // g_log.warning() << "HAB3 (LOWER) out[17087] = " << out[17086] << "\n";
  // g_log.warning() << "HAB3 (UPPER) out[17434] = " << out[17433] << "\n";
  // g_log.warning() << "HAB4 (LOWER) out[17437] = " << out[17436] << "\n";
  // g_log.warning() << "HAB4 (UPPER) out[17784] = " << out[17783] << "\n";

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
