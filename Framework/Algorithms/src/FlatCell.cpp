// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FlatCell.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/WorkspaceFactory.h"

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
  declareProperty("CreateMaskedBins", true, "If true, masked bins workspaces will be created.");
}

/** Masks the values in the vector if the are above a certain threshold
 */
void FlatCell::maskByThreshold(std::vector<double> &values, double threshold) {
  std::transform(values.begin(), values.end(), values.begin(),
                 [threshold](double v) { return (v > threshold) ? 0.0 : v; });
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
  double accum = std::accumulate(values.begin(), values.end(), 0.0, [m](double total, double x) {
    const double diff = x - m;
    return total + diff * diff;
  });
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
  bool createMaskedWorkspace = getProperty("CreateMaskedBins");

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

  const int lowAngleBankStart{2};
  const int lowAngleBankStop{16386};
  const int highAngleBankOneStop{16734};
  const int highAngleBankTwoStart{16736};
  const int highAngleBankTwoStop{17084};
  const int highAngleBankThreeStart{17086};
  const int highAngleBankThreeStop{17434};
  const int highAngleBankFourStart{17436};
  const int highAngleBankFourStop{17784};
  const int highAngleBankStop{17786};
  const int Ysize{17992};

  // Save the Low and High Angle Bank values into vectors
  std::vector<double> LAB(Y.begin() + lowAngleBankStart, Y.begin() + lowAngleBankStop);
  std::vector<double> HAB(Y.begin() + lowAngleBankStop, Y.begin() + highAngleBankStop);

  // const double StdLAB = FlatCell::stddev(LAB);
  // g_log.warning() << "StdLAB = " << StdLAB << "\n";

  // Normalize the values in the LAB and HAB vectors
  FlatCell::scale(LAB, 1 / FlatCell::mean(LAB));
  FlatCell::scale(HAB, 1 / FlatCell::mean(HAB));

  // Calculate the normalized mean of the LAB and HAB
  const double normMeanLAB = FlatCell::mean(LAB);
  const double normMeanHAB = FlatCell::mean(HAB);

  // Calculate the normalized std of the LAB and HAB
  const double normStdLAB = FlatCell::stddev(LAB);
  const double normStdHAB = FlatCell::stddev(HAB);

  // Create the array to save the output
  std::vector<double> out(Ysize, 0.0);
  std::copy(LAB.begin(), LAB.end(), out.begin() + lowAngleBankStart);
  std::copy(HAB.begin(), HAB.end(), out.begin() + lowAngleBankStop);

  std::vector<double> HAB1(out.begin() + lowAngleBankStop, out.begin() + highAngleBankOneStop);
  std::vector<double> HAB2(out.begin() + highAngleBankTwoStart, out.begin() + highAngleBankTwoStop);
  std::vector<double> HAB3(out.begin() + highAngleBankThreeStart, out.begin() + highAngleBankThreeStop);
  std::vector<double> HAB4(out.begin() + highAngleBankFourStart, out.begin() + highAngleBankFourStop);

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
  std::copy(HAB1.begin(), HAB1.end(), out.begin() + lowAngleBankStop);
  std::copy(HAB2.begin(), HAB2.end(), out.begin() + highAngleBankTwoStart);
  std::copy(HAB3.begin(), HAB3.end(), out.begin() + highAngleBankThreeStart);
  std::copy(HAB4.begin(), HAB4.end(), out.begin() + highAngleBankFourStart);

  // Change the values of the edge cases
  out[16735] *= rescaleHAB2;
  out[17435] *= rescaleHAB4;
  out[17784] *= rescaleHAB4;

  // Save the Y data into the output WS
  auto &O = outputWS->mutableY(0);
  std::copy(out.begin(), out.end(), O.begin());

  // Update the E values
  std::vector<double> e(Ysize, 0.0);
  auto &E = outputWS->mutableE(0);
  std::copy(e.begin(), e.end(), E.begin());

  // Mask the bins and create a workspace
  if (createMaskedWorkspace) {

    // Create the LAB and HAB vectors to mask
    std::vector<double> unmaskedLAB(out.begin() + lowAngleBankStart, out.begin() + lowAngleBankStop);
    std::vector<double> unmaskedHAB(out.begin() + lowAngleBankStop, out.begin() + highAngleBankStop);

    // Calculate the thresholds
    const double maskingThresholdLAB = normMeanLAB + normStdLAB;
    const double maskingThresholdHAB = normMeanHAB + (0.5 * normStdHAB);

    // g_log.warning() << "normStdLAB = " << normStdLAB << "\n";
    // g_log.warning() << "normStdHAB = " << normStdHAB << "\n";

    // g_log.warning() << "maskingThresholdLAB = " << maskingThresholdLAB << "\n";
    // g_log.warning() << "maskingThresholdHAB = " << maskingThresholdHAB << "\n";

    // Mask the LAB and HAB
    FlatCell::maskByThreshold(unmaskedLAB, maskingThresholdLAB);
    FlatCell::maskByThreshold(unmaskedHAB, maskingThresholdHAB);

    // Copy each of the vectors to out
    std::copy(unmaskedLAB.begin(), unmaskedLAB.end(), out.begin() + lowAngleBankStart);
    std::copy(unmaskedHAB.begin(), unmaskedHAB.end(), out.begin() + lowAngleBankStop);

    // Create a workspace
    MatrixWorkspace_sptr maskedWS = outputWS->clone();

    // Save the Y data into the masked WS
    auto &M = maskedWS->mutableY(0);
    std::copy(out.begin(), out.end(), M.begin());

    // Add to the Analysis Data Service
    const std::string baseName = getPropertyValue("OutputWorkspace");
    const std::string maskedName = baseName + "_Masked";
    AnalysisDataService::Instance().addOrReplace(maskedName, maskedWS);
  }
}

/** Execution code for EventWorkspaces
 */
void FlatCell::execEvent() {
  // To Do
}

} // namespace Mantid::Algorithms
