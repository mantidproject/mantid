// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GenerateFlatCellWorkspaceLOQ.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/IDTypes.h"

#include <fstream>
#include <limits>
#include <span>
#include <sstream>

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GenerateFlatCellWorkspaceLOQ)

using namespace Kernel;
using namespace API;
using namespace Mantid::DataObjects;

struct LoqMeta {
  static constexpr int histograms() { return 17776; }
  static constexpr int LABIndexStart() { return 0; }
  static constexpr int LABTotalBanks() { return 16384; }
  static constexpr int HABTotalBanks() { return 1392; }
  static constexpr int HABIndividualBanks() { return 348; };
  static constexpr int HABIndexStart() { return 16384; };
  static constexpr int HAB1IndexStop() { return 16732; };
  static constexpr int HAB2IndexStop() { return 17080; };
  static constexpr int HAB3IndexStop() { return 17428; };
};

void GenerateFlatCellWorkspaceLOQ::init() {
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input event workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output event workspace.");
  declareProperty("CreateMaskWorkspace", true, "Determines if masked workspace needs to be created.");
  declareProperty("LABThresholdMultiplier", 1.0,
                  "The parameter that is used to scale the standard deviation in order to set the masking threshold of "
                  "the low angle bank.");
  declareProperty("HABThresholdMultiplier", 0.5,
                  "The parameter that is used to scale the standard deviation in order to set the masking threshold of "
                  "the high angle bank.");
  declareProperty("ApplyMaskDirectlyToWorkspace", false, "Determines if mask is directly applied to workspace.");
  declareProperty("OutputMaskFilePath", "",
                  "Path to the detector mask XML file. It must be provided for the algorithm to create the xml file.");
}

/** Computes the mean of the input span
 */
double GenerateFlatCellWorkspaceLOQ::mean(std::span<const double> values) const {
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

/** Computes the standard deviation of the input span
 */
double GenerateFlatCellWorkspaceLOQ::stddev(std::span<double> values) const {
  double m = mean(values);
  double accum = std::accumulate(values.begin(), values.end(), 0.0, [m](double total, double x) {
    const double diff = x - m;
    return total + diff * diff;
  });
  return std::sqrt(accum / static_cast<double>(values.size()));
}

/** Scales each of the elements of the input vector
 */
void GenerateFlatCellWorkspaceLOQ::scale(std::span<double> values, double factor) const {
  std::transform(values.begin(), values.end(), values.begin(), [factor](double x) { return x * factor; });
}

/** Execution code.
 *  @throw std::runtime_error If nHist not equal to 17776.
 */
void GenerateFlatCellWorkspaceLOQ::exec() {

  // Get the input WS
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Validate the number of histograms in the Input WS
  const size_t nHist = inputWS->getNumberHistograms();
  if (nHist != LoqMeta::histograms()) {
    throw std::runtime_error(
        "The expected number of histograms in the event workspace should be 17776 for SANS ISIS reduction of LOQ.");
  }

  // Create output workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS, LoqMeta::histograms(), 1, 1);
  setProperty("OutputWorkspace", outputWS);

  // Integrate spectrums in input events workspace
  auto processedWS = integrateInput(inputWS);

  // Extract the spectrums into a vector
  auto values = extractIntegratedValues(processedWS);
  std::span<double> valuesSpan(values);

  // Normalize the banks
  FlatCellStats stats = normalizeBanks(valuesSpan);

  // Save the Y data into the output WS
  for (size_t i = 0; i < nHist; ++i) {
    auto &Y = outputWS->mutableY(i);
    Y[0] = valuesSpan[i];
  }

  createAndSaveMaskWorkspace(outputWS, stats.normStdLAB, stats.normStdHAB);
}

GenerateFlatCellWorkspaceLOQ::FlatCellStats
GenerateFlatCellWorkspaceLOQ::normalizeBanks(std::span<double> values) const {
  // Save the Low and High Angle Bank values into spans
  std::span<double> LAB = values.subspan(LoqMeta::LABIndexStart(), LoqMeta::LABTotalBanks());
  std::span<double> HAB = values.subspan(LoqMeta::HABIndexStart(), LoqMeta::HABTotalBanks());

  // Normalize the values in the LAB and HAB vectors
  scale(LAB, 1.0 / mean(LAB));
  scale(HAB, 1.0 / mean(HAB));

  // Calculate the normalized std of the LAB and HAB
  const double normStdLAB = stddev(LAB);
  const double normStdHAB = stddev(HAB);

  // Save the individual High Angle Bank values into spans
  std::span<double> HAB1 = values.subspan(LoqMeta::HABIndexStart(), LoqMeta::HABIndividualBanks());
  std::span<double> HAB2 = values.subspan(LoqMeta::HAB1IndexStop(), LoqMeta::HABIndividualBanks());
  std::span<double> HAB3 = values.subspan(LoqMeta::HAB2IndexStop(), LoqMeta::HABIndividualBanks());
  std::span<double> HAB4 = values.subspan(LoqMeta::HAB3IndexStop(), LoqMeta::HABIndividualBanks());

  // Rescale the values in the HAB vectors
  scale(HAB1, 1.0 / mean(HAB1));
  scale(HAB2, 1.0 / mean(HAB2));
  scale(HAB3, 1.0 / mean(HAB3));
  scale(HAB4, 1.0 / mean(HAB4));

  return {normStdLAB, normStdHAB};
}

std::vector<double> GenerateFlatCellWorkspaceLOQ::extractIntegratedValues(const API::MatrixWorkspace_sptr &ws) const {
  const size_t nHist = ws->getNumberHistograms();
  std::vector<double> values;
  values.reserve(nHist);
  for (size_t i = 0; i < nHist; ++i) {
    const auto &Y = ws->readY(i);
    values.insert(values.end(), Y.begin(), Y.end());
  }
  return values;
}

MatrixWorkspace_sptr GenerateFlatCellWorkspaceLOQ::integrateInput(const Workspace_sptr &ws) {
  auto integration = createChildAlgorithm("Integration");
  integration->initialize();
  integration->setProperty("InputWorkspace", ws);
  integration->setProperty("OutputWorkspace", "processedWS");
  integration->setProperty("StartWorkspaceIndex", LoqMeta::LABIndexStart());
  integration->setProperty("EndWorkspaceIndex", LoqMeta::histograms() - 1);
  integration->execute();
  return integration->getProperty("OutputWorkspace");
}

void GenerateFlatCellWorkspaceLOQ::createAndSaveMaskWorkspace(const MatrixWorkspace_sptr &ws, double normStdLAB,
                                                              double normStdHAB) {
  MatrixWorkspace_sptr directMaskWS = ws->clone();

  // Calculate the thresholds
  double labThresholdMultiplier = getProperty("LABThresholdMultiplier");
  double habThresholdMultiplier = getProperty("HABThresholdMultiplier");
  const double maskingThresholdLAB = 1.0 + (labThresholdMultiplier * normStdLAB);
  const double maskingThresholdHAB = 1.0 + (habThresholdMultiplier * normStdHAB);

  // Mask the values of the low angle bank
  auto maskDetectorsLAB = createChildAlgorithm("MaskDetectorsIf");
  maskDetectorsLAB->initialize();
  maskDetectorsLAB->setProperty("InputWorkspace", directMaskWS);
  maskDetectorsLAB->setProperty("OutputWorkspace", directMaskWS);
  maskDetectorsLAB->setProperty("StartWorkspaceIndex", LoqMeta::LABIndexStart());
  maskDetectorsLAB->setProperty("EndWorkspaceIndex", LoqMeta::LABTotalBanks() - 1);
  maskDetectorsLAB->setProperty("Operator", "GreaterEqual");
  maskDetectorsLAB->setProperty("Value", maskingThresholdLAB);
  maskDetectorsLAB->execute();

  // Mask the values of the high angle bank
  auto maskDetectorsHAB = createChildAlgorithm("MaskDetectorsIf");
  maskDetectorsHAB->initialize();
  maskDetectorsHAB->setProperty("InputWorkspace", directMaskWS);
  maskDetectorsHAB->setProperty("OutputWorkspace", directMaskWS);
  maskDetectorsHAB->setProperty("StartWorkspaceIndex", LoqMeta::HABIndexStart());
  maskDetectorsHAB->setProperty("Operator", "GreaterEqual");
  maskDetectorsHAB->setProperty("Value", maskingThresholdHAB);
  maskDetectorsHAB->execute();

  // Extract Mask
  const std::string maskName = getPropertyValue("OutputWorkspace") + "_MASK";
  auto extractMask = createChildAlgorithm("ExtractMask");
  extractMask->initialize();
  extractMask->setProperty("InputWorkspace", directMaskWS);
  extractMask->setProperty("OutputWorkspace", maskName);
  extractMask->execute();
  MatrixWorkspace_sptr extractedmaskWS = extractMask->getProperty("OutputWorkspace");

  // Save Mask
  if (!isDefault("OutputMaskFilePath")) {
    std::string outputMaskFilePath = getProperty("OutputMaskFilePath");
    auto saveMask = createChildAlgorithm("SaveMask");
    saveMask->initialize();
    saveMask->setProperty("InputWorkspace", extractedmaskWS);
    saveMask->setProperty("OutputFile", outputMaskFilePath);
    saveMask->execute();
  }

  bool applyMaskDirectlyToWorkspace = getProperty("ApplyMaskDirectlyToWorkspace");
  if (applyMaskDirectlyToWorkspace) {
    setProperty("OutputWorkspace", directMaskWS);
  }
  bool createMaskWorkspace = getProperty("CreateMaskWorkspace");
  if (createMaskWorkspace) {
    AnalysisDataService::Instance().addOrReplace(maskName, extractedmaskWS);
  }
}

/** Execution code for EventWorkspaces
 */
void GenerateFlatCellWorkspaceLOQ::execEvent() { exec(); }

} // namespace Mantid::Algorithms
