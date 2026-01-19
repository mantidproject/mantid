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
  static constexpr int LABTotalBanks() { return 16384; }
  static constexpr int HABTotalBanks() { return 1392; }
  static constexpr int HABIndividualBanks() { return 348; };
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
  declareProperty("CreateMaskedWorkspace", true, "Determines if masked workspace needs to be created.");
  declareProperty("MaskFileName", "FlatCellMasked.xml", "Path to the detector mask XML file.");
}

/** Computes the mean of the input vector
 */
double FlatCell::mean(std::span<const double> values) const {
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

/** Computes the standard deviation of the input vector
 */
double FlatCell::stddev(std::span<double> values) const {
  double m = mean(values);
  double accum = std::accumulate(values.begin(), values.end(), 0.0, [m](double total, double x) {
    const double diff = x - m;
    return total + diff * diff;
  });
  return std::sqrt(accum / static_cast<double>(values.size()));
}

/** Scales each of the elements of the input vector
 */
void FlatCell::scale(std::span<double> values, double factor) {
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
  std::span<double> values_span(values);

  // Save the Low and High Angle Bank values into spans
  std::span<double> LAB = values_span.subspan(LoqMeta::LABIndexStart(), LoqMeta::LABTotalBanks());
  std::span<double> HAB = values_span.subspan(LoqMeta::HABIndexStart(), LoqMeta::HABTotalBanks());

  // Calculate the mean of the LAB and HAB
  const double meanLAB = FlatCell::mean(LAB);
  const double meanHAB = FlatCell::mean(HAB);

  // Normalize the values in the LAB and HAB vectors
  FlatCell::scale(LAB, 1.0 / meanLAB);
  FlatCell::scale(HAB, 1.0 / meanHAB);

  // Calculate the normalized std of the LAB and HAB
  const double normStdLAB = FlatCell::stddev(LAB);
  const double normStdHAB = FlatCell::stddev(HAB);

  // Create the Output Y and X
  std::vector<double> maskedOutY(LoqMeta::histograms(), 0.0);
  std::vector<detid_t> outX(LoqMeta::histograms());

  // Map the spectrum ids to the detector ids for the Low and High Angle Banks
  std::iota(outX.begin(), outX.begin() + LAB.size(), LoqMeta::LABDetectorIdStart());
  std::iota(outX.begin() + LoqMeta::HABIndexStart(),
            outX.begin() + LoqMeta::HABIndexStart() + LoqMeta::HABIndividualBanks(), LoqMeta::HAB1DetectorIdStart());
  std::iota(outX.begin() + LoqMeta::HAB1IndexStop(),
            outX.begin() + LoqMeta::HAB1IndexStop() + LoqMeta::HABIndividualBanks(), LoqMeta::HAB2DetectorIdStart());
  std::iota(outX.begin() + LoqMeta::HAB2IndexStop(),
            outX.begin() + LoqMeta::HAB2IndexStop() + LoqMeta::HABIndividualBanks(), LoqMeta::HAB3DetectorIdStart());
  std::iota(outX.begin() + LoqMeta::HAB3IndexStop(),
            outX.begin() + LoqMeta::HAB3IndexStop() + LoqMeta::HABIndividualBanks(), LoqMeta::HAB4DetectorIdStart());

  // Save the individual High Angle Bank values into spans
  std::span<double> HAB1 = values_span.subspan(LoqMeta::HABIndexStart(), LoqMeta::HABIndividualBanks());
  std::span<double> HAB2 = values_span.subspan(LoqMeta::HAB1IndexStop(), LoqMeta::HABIndividualBanks());
  std::span<double> HAB3 = values_span.subspan(LoqMeta::HAB2IndexStop(), LoqMeta::HABIndividualBanks());
  std::span<double> HAB4 = values_span.subspan(LoqMeta::HAB3IndexStop(), LoqMeta::HABIndividualBanks());

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

  // Save the Y data into the output WS
  auto &OY = outputWS->mutableY(0);
  std::copy(values_span.begin(), values_span.end(), OY.begin() + LoqMeta::nMonitorOffset());

  // Save the X data into the output WS
  auto &OX = outputWS->mutableX(0);
  std::copy(outX.begin(), outX.end(), OX.begin() + LoqMeta::nMonitorOffset());

  // Create the LAB and HAB vectors to mask
  std::vector<double> unmaskedLAB(values_span.begin() + LoqMeta::LABIndexStart(),
                                  values_span.begin() + LoqMeta::LABIndexStop());
  std::vector<double> unmaskedHAB(values_span.begin() + LoqMeta::HABIndexStart(),
                                  values_span.begin() + LoqMeta::HABIndexStop());

  bool CreateMaskedWorkspace = getProperty("CreateMaskedWorkspace");

  if (CreateMaskedWorkspace) {

    // Calculate the thresholds
    const double maskingThresholdLAB = 1 + normStdLAB;
    const double maskingThresholdHAB = 1 + (0.5 * normStdHAB);

    // Crop the workspaces
    auto cropWorkspace = createChildAlgorithm("CropWorkspace");
    cropWorkspace->initialize();

    // Crop Workspace for low angle bank
    cropWorkspace->setProperty("InputWorkspace", outputWS);
    cropWorkspace->setProperty("OutputWorkspace", "labCroppedWS");
    cropWorkspace->setProperty("XMin", static_cast<double>(LoqMeta::LABIndexStart()));
    cropWorkspace->setProperty("XMax",
                               static_cast<double>(LoqMeta::LABDetectorIdStart() + LoqMeta::LABTotalBanks() - 1));
    cropWorkspace->execute();
    MatrixWorkspace_sptr labCroppedWS = cropWorkspace->getProperty("OutputWorkspace");

    // Crop Workspace for high angle bank
    cropWorkspace->setProperty("InputWorkspace", outputWS);
    cropWorkspace->setProperty("OutputWorkspace", "habCroppedWS");
    cropWorkspace->setProperty("XMin", static_cast<double>(LoqMeta::HAB1DetectorIdStart()));
    cropWorkspace->setProperty("XMax",
                               static_cast<double>(LoqMeta::HAB4DetectorIdStart() + LoqMeta::HABIndividualBanks() - 1));
    cropWorkspace->execute();
    MatrixWorkspace_sptr habCroppedWS = cropWorkspace->getProperty("OutputWorkspace");

    // Mask the values of the low angle bank
    auto maskDetectorsIf = createChildAlgorithm("MaskDetectorsIf");
    maskDetectorsIf->initialize();
    maskDetectorsIf->setProperty("InputWorkspace", labCroppedWS);
    maskDetectorsIf->setProperty("OutputWorkspace", labCroppedWS);
    maskDetectorsIf->setProperty("Operator", "GreaterEqual");
    maskDetectorsIf->setProperty("Value", maskingThresholdLAB);
    maskDetectorsIf->execute();

    maskDetectorsIf->setProperty("InputWorkspace", habCroppedWS);
    maskDetectorsIf->setProperty("OutputWorkspace", habCroppedWS);
    maskDetectorsIf->setProperty("Operator", "GreaterEqual");
    maskDetectorsIf->setProperty("Value", maskingThresholdHAB);
    maskDetectorsIf->execute();

    // Coinjoin Workspaces
    auto conjoinWorkspaces = createChildAlgorithm("ConjoinWorkspaces");
    conjoinWorkspaces->initialize();
    conjoinWorkspaces->setProperty("InputWorkspace1", labCroppedWS);
    conjoinWorkspaces->setProperty("InputWorkspace2", habCroppedWS);
    conjoinWorkspaces->setProperty("OutputWorkspace", "maskedWS");
    conjoinWorkspaces->execute();
    MatrixWorkspace_sptr maskedWS = conjoinWorkspaces->getProperty("OutputWorkspace");
  };

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
