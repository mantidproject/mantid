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
#include "MantidGeometry/IDTypes.h"

#include <fstream>
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
  declareProperty("MaskFileName", "FlatCellMasked.xml", "Path to the detector mask XML file.");
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

  // Get the input WS
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Spectrum Numbers go from 9-17784
  // Workspace Indices go from 0-17775

  // Validate the number of histograms in the Input WS
  const size_t nHist = inputWS->getNumberHistograms();
  if (nHist == 0) {
    throw std::runtime_error("Input workspace has no histograms. The input should have 17776 histograms.");
  }
  if (nHist != 17776) {
    throw std::runtime_error("Expecting the input workspace to have 17776 histograms.");
  }

  // The output is expected to have 17784 values (nHist+nMonitorOffset)
  const int nMonitorOffset{8};
  const int totalDetectorIDs{17784};
  MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create("Workspace2D", 1, totalDetectorIDs, totalDetectorIDs);
  setProperty("OutputWorkspace", outputWS);

  // Clone input before rebinning
  EventWorkspace_sptr rebinnedWS = inputWS->clone();

  // Rebin input to one spectrum
  auto rebin = createChildAlgorithm("Rebin");
  rebin->initialize();
  rebin->setProperty("InputWorkspace", rebinnedWS);
  rebin->setProperty("OutputWorkspace", rebinnedWS);
  rebin->setProperty("Params", "43500");
  rebin->execute();

  // Extract the spectrums into a vector
  std::vector<double> values;
  for (size_t i = 0; i < nHist; ++i) {
    const auto &Y = rebinnedWS->readY(i);
    values.insert(values.end(), Y.begin(), Y.end());
  }

  // Define the constants used to define the start
  // and stop of the high angle and low angle banks
  constexpr size_t LABStart{0};
  constexpr size_t LABStop{16384};

  // Adjusting start values from the excel file because of the spectrum vs indices confusion
  constexpr size_t HABStart{16392 - nMonitorOffset};
  constexpr size_t HAB1Stop{16740 - nMonitorOffset};
  constexpr size_t HAB2Stop{17088 - nMonitorOffset};
  constexpr size_t HAB3Stop{17436 - nMonitorOffset};
  constexpr size_t HABStop{17784 - nMonitorOffset};

  // Save the Low and High Angle Bank values into vectors
  std::vector<double> LAB(values.begin() + LABStart, values.begin() + LABStop);
  std::vector<double> HAB(values.begin() + HABStart, values.begin() + HABStop);

  // Calculate the mean of the LAB and HAB
  const double meanLAB = FlatCell::mean(std::vector<double>(LAB.begin(), LAB.end()));
  const double meanHAB = FlatCell::mean(std::vector<double>(HAB.begin(), HAB.end()));

  // Normalize the values in the LAB and HAB vectors
  FlatCell::scale(LAB, 1 / meanLAB);
  FlatCell::scale(HAB, 1 / meanHAB);

  // Calculate the normalized std of the LAB and HAB
  const double normStdLAB = FlatCell::stddev(LAB);
  const double normStdHAB = FlatCell::stddev(HAB);

  // Create the Output Y and X
  std::vector<double> outY(HABStop, 0.0);
  std::vector<double> maskedOutY(HABStop, 0.0);
  std::vector<detid_t> outX(HABStop);

  // Map the spectrum ids to the detector ids for the Low and High Angle Banks
  std::iota(outX.begin(), outX.begin() + LAB.size(), 100000);
  std::iota(outX.begin() + HABStart, outX.begin() + HABStart + 348, 200000);
  std::iota(outX.begin() + HAB1Stop, outX.begin() + HAB1Stop + 348, 210000);
  std::iota(outX.begin() + HAB2Stop, outX.begin() + HAB2Stop + 348, 220000);
  std::iota(outX.begin() + HAB3Stop, outX.begin() + HAB3Stop + 348, 230000);

  std::copy(LAB.begin(), LAB.end(), outY.begin() + LABStart);
  std::copy(HAB.begin(), HAB.end(), outY.begin() + HABStart);

  std::vector<double> HAB1(outY.begin() + HABStart, outY.begin() + HAB1Stop);
  std::vector<double> HAB2(outY.begin() + HAB1Stop, outY.begin() + HAB2Stop);
  std::vector<double> HAB3(outY.begin() + HAB2Stop, outY.begin() + HAB3Stop);
  std::vector<double> HAB4(outY.begin() + HAB3Stop, outY.begin() + HABStop);

  // Calculate the rescale factor of each high angle bank
  double rescaleHAB1 = 1 / FlatCell::mean(HAB1);
  double rescaleHAB2 = 1 / FlatCell::mean(HAB2);
  double rescaleHAB3 = 1 / FlatCell::mean(HAB3);
  double rescaleHAB4 = 1 / FlatCell::mean(HAB4);

  // Rescale the values in the HAB vectors
  FlatCell::scale(HAB1, rescaleHAB1);
  FlatCell::scale(HAB2, rescaleHAB2);
  FlatCell::scale(HAB3, rescaleHAB3);
  FlatCell::scale(HAB4, rescaleHAB4);

  // Copy the values in the output spectrum
  std::copy(HAB1.begin(), HAB1.end(), outY.begin() + HABStart);
  std::copy(HAB2.begin(), HAB2.end(), outY.begin() + HAB1Stop);
  std::copy(HAB3.begin(), HAB3.end(), outY.begin() + HAB2Stop);
  std::copy(HAB4.begin(), HAB4.end(), outY.begin() + HAB3Stop);

  // Save the Y data into the output WS
  auto &OY = outputWS->mutableY(0);
  std::copy(outY.begin(), outY.end(), OY.begin() + nMonitorOffset);

  // Save the X data into the output WS
  auto &OX = outputWS->mutableX(0);
  std::copy(outX.begin(), outX.end(), OX.begin() + nMonitorOffset);

  // Create the LAB and HAB vectors to mask
  std::vector<double> unmaskedLAB(outY.begin() + LABStart, outY.begin() + LABStop);
  std::vector<double> unmaskedHAB(outY.begin() + HABStart, outY.begin() + HABStop);

  // Calculate the thresholds
  const double maskingThresholdLAB = 1 + normStdLAB;
  const double maskingThresholdHAB = 1 + (0.5 * normStdHAB);

  // Mask the LAB and HAB
  FlatCell::maskByThreshold(unmaskedLAB, maskingThresholdLAB);
  FlatCell::maskByThreshold(unmaskedHAB, maskingThresholdHAB);

  // Copy each of the vectors to outY
  std::copy(unmaskedLAB.begin(), unmaskedLAB.end(), maskedOutY.begin() + LABStart);
  std::copy(unmaskedHAB.begin(), unmaskedHAB.end(), maskedOutY.begin() + HABStart);

  // Determine which detector IDs need to be masked
  std::vector<detid_t> detectorIDs;
  for (size_t i = 0; i < maskedOutY.size(); ++i) {
    if (maskedOutY[i] == 0.0) {
      detectorIDs.push_back(outX[i]);
    }
  }

  // Create the string stream for writing into the xml file
  std::ostringstream detids_stream;
  for (size_t i = 0; i < detectorIDs.size(); ++i) {
    detids_stream << detectorIDs[i];
    if (i + 1 < detectorIDs.size()) {
      detids_stream << ",";
    }
  }

  // Create the xml file
  std::string maskFileName = getProperty("MaskFileName");
  std::ofstream outFile(maskFileName);
  if (!outFile) {
    throw std::runtime_error("Failed to open mask file for writing");
  }

  // Write detector ids to the xml file
  outFile << "<?xml version=\"1.0\"?>\n";
  outFile << "<detector-masking>\n";
  outFile << "  <group>\n";
  outFile << "    <detids>" << detids_stream.str() << "</detids>\n";
  outFile << "  </group>\n";
  outFile << "</detector-masking>\n";

  // Close the file
  outFile.close();
}

/** Execution code for EventWorkspaces
 */
void FlatCell::execEvent() { exec(); }

} // namespace Mantid::Algorithms
