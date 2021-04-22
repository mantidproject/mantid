// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RemoveSpectra.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Indexing;

DECLARE_ALGORITHM(RemoveSpectra)

void RemoveSpectra::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input));
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output));
  declareProperty(std::make_unique<ArrayProperty<size_t>>("WorkspaceIndices", Direction::Input),
                  "A comma-separated list of individual workspace indices to remove");
  declareProperty("RemoveMaskedSpectra", false,
                  "Whether or not to remove spectra that have been masked from "
                  "the inputworkspace.",
                  Direction::Input);
  declareProperty("RemoveSpectraWithNoDetector", false,
                  "Whether or not to remove spectra that have no attached detector.", Direction::Input);
}

std::map<std::string, std::string> RemoveSpectra::validateInputs() {
  std::map<std::string, std::string> errors;
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const auto workspace2D = std::dynamic_pointer_cast<const Workspace2D>(inputWS);
  const auto eventWS = std::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (!workspace2D && !eventWS) {
    errors.insert(std::make_pair("InputWorkspace", "A none-Workspace2D or EventWorkspace has been provided."));
  }

  const std::vector<size_t> indexList = getProperty("WorkspaceIndices");
  for (const auto &index : indexList) {
    // index >= 0 is assumed as it should be a const long unsinged int
    if (index >= inputWS->getNumberHistograms()) {
      errors.insert(std::make_pair("WorkspaceIndices", "Passed Workspace Index: " + std::to_string(index) +
                                                           " is not valid for passed InputWorkspace"));
    }
  }

  return errors;
}

namespace {
std::vector<size_t> discoverSpectraWithNoDetector(const MatrixWorkspace_sptr &inputWS) {
  std::vector<size_t> specIDs;
  const auto &spectrumInfo = inputWS->spectrumInfo();
  for (auto i = 0u; i < inputWS->getNumberHistograms(); ++i) {
    if (!spectrumInfo.hasDetectors(i))
      specIDs.emplace_back(i);
  }
  return specIDs;
}

std::vector<size_t> discoverSpectraWithMask(const MatrixWorkspace_sptr &inputWS) {
  std::vector<size_t> specIDs;
  const auto &spectrumInfo = inputWS->spectrumInfo();
  for (auto i = 0u; i < inputWS->getNumberHistograms(); ++i) {
    if (!spectrumInfo.hasDetectors(i))
      continue;
    if (spectrumInfo.isMasked(i)) {
      specIDs.emplace_back(i);
    }
  }
  return specIDs;
}

template <class T> bool evaluateIfSpectrumIsInList(std::vector<size_t> &specList, T spectrum) {
  const auto it = std::find(specList.begin(), specList.end(), spectrum->getSpectrumNo());
  return it != specList.end();
}
} // namespace

void RemoveSpectra::exec() {
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::vector<size_t> specList = getProperty("WorkspaceIndices");
  const bool removeMaskedSpectra = getProperty("RemoveMaskedSpectra");
  const bool removeSpectraWithNoDetector = getProperty("RemoveSpectraWithNoDetector");

  if (specList.empty() && removeMaskedSpectra && removeSpectraWithNoDetector) {
    g_log.warning("Nothing passed to the RemoveSpectra algorithm to remove so "
                  "nothing happened");
    setProperty("OutputWorkspace", inputWS);
    return;
  }

  if (removeMaskedSpectra) {
    const auto extraSpectra = discoverSpectraWithMask(inputWS);
    specList.insert(specList.end(), extraSpectra.begin(), extraSpectra.end());
  }

  if (removeSpectraWithNoDetector) {
    const auto extraSpectra = discoverSpectraWithNoDetector(inputWS);
    specList.insert(specList.end(), extraSpectra.begin(), extraSpectra.end());
  }

  if (specList.empty()) {
    g_log.debug("No spectra to delete in RemoveSpectra");
    setProperty("OutputWorkspace", inputWS);
    return;
  }

  auto outputWS = copySpectraFromInputToOutput(inputWS, specList);

  g_log.debug(std::to_string(specList.size()) + " spectra removed.");

  setProperty("OutputWorkspace", outputWS);
}

MatrixWorkspace_sptr RemoveSpectra::copySpectraFromInputToOutput(MatrixWorkspace_sptr inputWS,
                                                                 const std::vector<size_t> &specList) {
  std::vector<size_t> indicesToExtract;
  for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
    if (std::find(specList.begin(), specList.end(), i) == specList.end()) {
      indicesToExtract.emplace_back(i);
    }
  }

  if (indicesToExtract.empty()) {
    return inputWS;
  }

  auto extractSpectra = createChildAlgorithm("ExtractSpectra");
  extractSpectra->setProperty("InputWorkspace", inputWS);
  extractSpectra->setProperty("WorkspaceIndexList", indicesToExtract);
  extractSpectra->executeAsChildAlg();

  MatrixWorkspace_sptr outputWS = extractSpectra->getProperty("OutputWorkspace");
  return outputWS;
}
} // namespace Algorithms
} // namespace Mantid
