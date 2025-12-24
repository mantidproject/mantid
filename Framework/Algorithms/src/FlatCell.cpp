// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FlatCell.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"

#include <limits>
#include <sstream>

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FlatCell)

using namespace Kernel;
using namespace API;
using namespace Mantid::DataObjects;

void FlatCell::init() {
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input event workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output event workspace.");
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

  // Get the input and output WS
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const size_t nHist = inputWS->getNumberHistograms();
  const int nMonitorOffset{8};
  const int totalDetectorIDs{17784};
  MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create("Workspace2D", 1, totalDetectorIDs, totalDetectorIDs);
  setProperty("OutputWorkspace", outputWS);

  // Validate before rebinning
  EventWorkspace_sptr rebinnedWS = inputWS->clone();

  // Rebin to one spectrum
  auto rebin = createChildAlgorithm("Rebin");
  rebin->initialize();
  rebin->setProperty("InputWorkspace", rebinnedWS);
  rebin->setProperty("OutputWorkspace", rebinnedWS);
  rebin->setProperty("Params", "43500");
  rebin->execute();

  // Extract the spectrums into a vector
  std::vector<double> values;
  if (nHist == 0) {
    throw std::runtime_error("Input workspace has no histograms");
  }
  for (size_t i = 0; i < nHist; ++i) {
    const auto &Y = rebinnedWS->readY(i);
    values.insert(values.end(), Y.begin(), Y.end());
  }

  constexpr size_t lowAngleBankStart{0};
  constexpr size_t lowAngleBankStop{16386};
  constexpr size_t highAngleBankStop{17776};

  // Save the Low and High Angle Bank values into vectors
  std::vector<double> LAB(values.begin() + lowAngleBankStart, values.begin() + lowAngleBankStop);
  std::vector<double> HAB(values.begin() + lowAngleBankStop, values.begin() + highAngleBankStop);

  // Calculate the normalized mean of the LAB and HAB
  const double meanLAB = FlatCell::mean(std::vector<double>(LAB.begin(), LAB.end() - 2));
  const double meanHAB = FlatCell::mean(std::vector<double>(HAB.begin() + 6, HAB.end()));

  g_log.warning() << "Mean LAB: " << meanLAB << "\n";
  g_log.warning() << "Mean HAB: " << meanHAB << "\n";

  // Normalize the values in the LAB and HAB vectors
  FlatCell::scale(LAB, 1 / meanLAB);
  FlatCell::scale(HAB, 1 / meanHAB);

  // Calculate the normalized std of the LAB and HAB
  const double normStdLAB = FlatCell::stddev(LAB);
  const double normStdHAB = FlatCell::stddev(HAB);

  g_log.warning() << "Norm STD LAB: " << normStdLAB << "\n";
  g_log.warning() << "Norm STD HAB: " << normStdHAB << "\n";

  std::vector<double> out(highAngleBankStop, 0.0);
  std::copy(LAB.begin(), LAB.end(), out.begin() + lowAngleBankStart);
  std::copy(HAB.begin(), HAB.end(), out.begin() + lowAngleBankStop);

  const int highAngleBankOneStop{16735};
  const int highAngleBankTwoStop{17088};
  const int highAngleBankThreeStop{17436};

  std::vector<double> HAB1(out.begin() + lowAngleBankStop, out.begin() + highAngleBankOneStop);
  std::vector<double> HAB2(out.begin() + highAngleBankOneStop, out.begin() + highAngleBankTwoStop);
  std::vector<double> HAB3(out.begin() + highAngleBankTwoStop, out.begin() + highAngleBankThreeStop);
  std::vector<double> HAB4(out.begin() + highAngleBankThreeStop, out.begin() + highAngleBankStop);

  g_log.warning() << "Mean HAB-1: " << FlatCell::mean(std::vector<double>(HAB1.begin() + 6, HAB1.end())) << "\n";
  g_log.warning() << "Mean HAB-2: " << FlatCell::mean(std::vector<double>(HAB2.begin() + 5, HAB2.end())) << "\n";
  g_log.warning() << "Mean HAB-3: " << FlatCell::mean(HAB3) << "\n";
  g_log.warning() << "Mean HAB-4: " << FlatCell::mean(HAB4) << "\n";

  // Calculate the rescale factor of each high angle bank
  double rescaleHAB1 = 1 / FlatCell::mean(std::vector<double>(HAB1.begin() + 6, HAB1.end()));
  double rescaleHAB2 = 1 / FlatCell::mean(std::vector<double>(HAB2.begin() + 5, HAB2.end()));
  double rescaleHAB3 = 1 / FlatCell::mean(HAB3);
  double rescaleHAB4 = 1 / FlatCell::mean(HAB4);

  g_log.warning() << "Rescale Factor HAB-1: " << rescaleHAB1 << "\n";
  g_log.warning() << "Rescale Factor HAB-2: " << rescaleHAB2 << "\n";
  g_log.warning() << "Rescale Factor HAB-3: " << rescaleHAB3 << "\n";
  g_log.warning() << "Rescale Factor HAB-4: " << rescaleHAB4 << "\n";

  // Rescale the values in the HAB vectors
  FlatCell::scale(HAB1, rescaleHAB1);
  FlatCell::scale(HAB2, rescaleHAB2);
  FlatCell::scale(HAB3, rescaleHAB3);
  FlatCell::scale(HAB4, rescaleHAB4);

  // Copy the values in the output spectrum
  std::copy(HAB1.begin(), HAB1.end(), out.begin() + lowAngleBankStop);
  std::copy(HAB2.begin(), HAB2.end(), out.begin() + highAngleBankOneStop);
  std::copy(HAB3.begin(), HAB3.end(), out.begin() + highAngleBankTwoStop);
  std::copy(HAB4.begin(), HAB4.end(), out.begin() + highAngleBankThreeStop);

  // Save the Y data into the output WS
  auto &O = outputWS->mutableY(0);
  std::copy(out.begin(), out.end(), O.begin() + nMonitorOffset);

  // bool createMaskedWorkspace = getProperty("CreateMaskedBins");

  // // Change the values of the edge cases
  // out[16735] *= rescaleHAB2;
  // out[17435] *= rescaleHAB4;
  // out[17784] *= rescaleHAB4;

  // // Save the Y data into the output WS
  // auto &O = outputWS->mutableY(0);
  // std::copy(out.begin(), out.end(), O.begin());

  // // Update the E values
  // std::vector<double> e(Ysize, 0.0);
  // auto &E = outputWS->mutableE(0);
  // std::copy(e.begin(), e.end(), E.begin());

  // // Mask the bins and create a workspace
  // if (createMaskedWorkspace) {

  //   // Create the LAB and HAB vectors to mask
  //   std::vector<double> unmaskedLAB(out.begin() + lowAngleBankStart, out.begin() + lowAngleBankStop);
  //   std::vector<double> unmaskedHAB(out.begin() + lowAngleBankStop, out.begin() + highAngleBankStop);

  //   // Calculate the thresholds
  //   const double maskingThresholdLAB = normMeanLAB + normStdLAB;
  //   const double maskingThresholdHAB = normMeanHAB + (0.5 * normStdHAB);

  //   // g_log.warning() << "normStdLAB = " << normStdLAB << "\n";
  //   // g_log.warning() << "normStdHAB = " << normStdHAB << "\n";

  //   // g_log.warning() << "maskingThresholdLAB = " << maskingThresholdLAB << "\n";
  //   // g_log.warning() << "maskingThresholdHAB = " << maskingThresholdHAB << "\n";

  //   // Mask the LAB and HAB
  //   FlatCell::maskByThreshold(unmaskedLAB, maskingThresholdLAB);
  //   FlatCell::maskByThreshold(unmaskedHAB, maskingThresholdHAB);

  //   // Copy each of the vectors to out
  //   std::copy(unmaskedLAB.begin(), unmaskedLAB.end(), out.begin() + lowAngleBankStart);
  //   std::copy(unmaskedHAB.begin(), unmaskedHAB.end(), out.begin() + lowAngleBankStop);

  //   // Create a workspace
  //   MatrixWorkspace_sptr maskedWS = outputWS->clone();

  //   // Save the Y data into the masked WS
  //   auto &M = maskedWS->mutableY(0);
  //   std::copy(out.begin(), out.end(), M.begin());

  //   // Add to the Analysis Data Service
  //   const std::string baseName = getPropertyValue("OutputWorkspace");
  //   const std::string maskedName = baseName + "_Masked";
  //   AnalysisDataService::Instance().addOrReplace(maskedName, maskedWS);
  // }
}

/** Execution code for EventWorkspaces
 */
void FlatCell::execEvent() { exec(); }

} // namespace Mantid::Algorithms
