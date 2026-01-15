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
#include <span>
#include <sstream>

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FlatCell)

using namespace Kernel;
using namespace API;
using namespace Mantid::DataObjects;

struct LoqMeta {
  static constexpr int totalDetectorIDs() { return 17784; }
  static constexpr int histograms() { return 17776; }
  static constexpr int nMonitorOffset() { return 8; }
  static constexpr int LABIndexStart() { return 0; }
  static constexpr int LABIndexStop() { return 16384; }
  static constexpr int HABReadings() { return 348; };
  static constexpr int HABIndexStart() { return 16392 - nMonitorOffset(); };
  static constexpr int HAB1IndexStop() { return 16740 - nMonitorOffset(); };
  static constexpr int HAB2IndexStop() { return 17088 - nMonitorOffset(); };
  static constexpr int HAB3IndexStop() { return 17436 - nMonitorOffset(); };
  static constexpr int HABIndexStop() { return 17784 - nMonitorOffset(); };
  static constexpr int LABDetectorIdStart() { return 100000; };
  static constexpr int HAB1DetectorIdStart() { return 200000; };
  static constexpr int HAB2DetectorIdStart() { return 210000; };
  static constexpr int HAB3DetectorIdStart() { return 220000; };
  static constexpr int HAB4DetectorIdStart() { return 230000; };
};

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
double FlatCell::mean(std::span<const double> values) const {
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

/** Computes the standard deviation of the input vector
 */
double FlatCell::stddev(std::span<const double> values) const {
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
 *  @throw std::runtime_error If nHist not equal to 17776.
 */
void FlatCell::exec() {

  // Get the input WS
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Validate the number of histograms in the Input WS
  const size_t nHist = inputWS->getNumberHistograms();
  if (nHist != LoqMeta::histograms()) {
    throw std::runtime_error(
        "The expected number of histograms in the event workspace should be 17776 for SANS ISIS reduction of LOQ.");
  }

  // The output is expected to have 17784 values (nHist+nMonitorOffset)
  MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create("Workspace2D", 1, LoqMeta::totalDetectorIDs(), LoqMeta::totalDetectorIDs());
  setProperty("OutputWorkspace", outputWS);

  // Integrate spectrums in input events workspace into one spectrum
  auto integration = createChildAlgorithm("Integration");
  integration->initialize();
  integration->setProperty("InputWorkspace", inputWS);
  integration->setProperty("OutputWorkspace", "processedWS");
  integration->setProperty("StartWorkspaceIndex", LoqMeta::LABIndexStart());
  integration->setProperty("EndWorkspaceIndex", LoqMeta::histograms() - 1);
  integration->execute();

  // Retrieve the integrated workspace
  MatrixWorkspace_sptr processedWS = integration->getProperty("OutputWorkspace");

  // Extract the spectrums into a vector
  std::vector<double> values;
  for (size_t i = 0; i < nHist; ++i) {
    const auto &Y = processedWS->readY(i);
    values.insert(values.end(), Y.begin(), Y.end());
  }

  // Save the Low and High Angle Bank values into vectors
  std::vector<double> LAB(values.begin() + LoqMeta::LABIndexStart(), values.begin() + LoqMeta::LABIndexStop());
  std::vector<double> HAB(values.begin() + LoqMeta::HABIndexStart(), values.begin() + LoqMeta::HABIndexStop());

  // Calculate the mean of the LAB and HAB
  const double meanLAB = FlatCell::mean(std::span<const double>(LAB));
  const double meanHAB = FlatCell::mean(std::span<const double>(HAB));

  // Normalize the values in the LAB and HAB vectors
  FlatCell::scale(LAB, 1 / meanLAB);
  FlatCell::scale(HAB, 1 / meanHAB);

  // Calculate the normalized std of the LAB and HAB
  const double normStdLAB = FlatCell::stddev(std::span<const double>(LAB));
  const double normStdHAB = FlatCell::stddev(std::span<const double>(HAB));

  // Create the Output Y and X
  std::vector<double> outY(LoqMeta::histograms(), 0.0);
  std::vector<double> maskedOutY(LoqMeta::histograms(), 0.0);
  std::vector<detid_t> outX(LoqMeta::histograms());

  // Map the spectrum ids to the detector ids for the Low and High Angle Banks
  std::iota(outX.begin(), outX.begin() + LAB.size(), LoqMeta::LABDetectorIdStart());
  std::iota(outX.begin() + LoqMeta::HABIndexStart(), outX.begin() + LoqMeta::HABIndexStart() + LoqMeta::HABReadings(),
            LoqMeta::HAB1DetectorIdStart());
  std::iota(outX.begin() + LoqMeta::HAB1IndexStop(), outX.begin() + LoqMeta::HAB1IndexStop() + LoqMeta::HABReadings(),
            LoqMeta::HAB2DetectorIdStart());
  std::iota(outX.begin() + LoqMeta::HAB2IndexStop(), outX.begin() + LoqMeta::HAB2IndexStop() + LoqMeta::HABReadings(),
            LoqMeta::HAB3DetectorIdStart());
  std::iota(outX.begin() + LoqMeta::HAB3IndexStop(), outX.begin() + LoqMeta::HAB3IndexStop() + LoqMeta::HABReadings(),
            LoqMeta::HAB4DetectorIdStart());

  std::copy(LAB.begin(), LAB.end(), outY.begin() + LoqMeta::LABIndexStart());
  std::copy(HAB.begin(), HAB.end(), outY.begin() + LoqMeta::HABIndexStart());

  std::vector<double> HAB1(outY.begin() + LoqMeta::HABIndexStart(), outY.begin() + LoqMeta::HAB1IndexStop());
  std::vector<double> HAB2(outY.begin() + LoqMeta::HAB1IndexStop(), outY.begin() + LoqMeta::HAB2IndexStop());
  std::vector<double> HAB3(outY.begin() + LoqMeta::HAB2IndexStop(), outY.begin() + LoqMeta::HAB3IndexStop());
  std::vector<double> HAB4(outY.begin() + LoqMeta::HAB3IndexStop(), outY.begin() + LoqMeta::HABIndexStop());

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
  std::copy(HAB1.begin(), HAB1.end(), outY.begin() + LoqMeta::HABIndexStart());
  std::copy(HAB2.begin(), HAB2.end(), outY.begin() + LoqMeta::HAB1IndexStop());
  std::copy(HAB3.begin(), HAB3.end(), outY.begin() + LoqMeta::HAB2IndexStop());
  std::copy(HAB4.begin(), HAB4.end(), outY.begin() + LoqMeta::HAB3IndexStop());

  // Save the Y data into the output WS
  auto &OY = outputWS->mutableY(0);
  std::copy(outY.begin(), outY.end(), OY.begin() + LoqMeta::nMonitorOffset());

  // Save the X data into the output WS
  auto &OX = outputWS->mutableX(0);
  std::copy(outX.begin(), outX.end(), OX.begin() + LoqMeta::nMonitorOffset());

  // Create the LAB and HAB vectors to mask
  std::vector<double> unmaskedLAB(outY.begin() + LoqMeta::LABIndexStart(), outY.begin() + LoqMeta::LABIndexStop());
  std::vector<double> unmaskedHAB(outY.begin() + LoqMeta::HABIndexStart(), outY.begin() + LoqMeta::HABIndexStop());

  // Calculate the thresholds
  const double maskingThresholdLAB = 1 + normStdLAB;
  const double maskingThresholdHAB = 1 + (0.5 * normStdHAB);

  // Mask the LAB and HAB
  FlatCell::maskByThreshold(unmaskedLAB, maskingThresholdLAB);
  FlatCell::maskByThreshold(unmaskedHAB, maskingThresholdHAB);

  // Copy each of the vectors to outY
  std::copy(unmaskedLAB.begin(), unmaskedLAB.end(), maskedOutY.begin() + LoqMeta::LABIndexStart());
  std::copy(unmaskedHAB.begin(), unmaskedHAB.end(), maskedOutY.begin() + LoqMeta::HABIndexStart());

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
