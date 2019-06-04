// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/RemoveSpectra.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
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
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "InputWorkspace", "", Direction::Input));
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "OutputWorkspace", "", Direction::Output));
  declareProperty(
      std::make_unique<ArrayProperty<size_t>>("WorkspaceIndices",
                                              Direction::Input),
      "A comma-separated list of individual workspace indices to remove");
  declareProperty("RemoveMaskedSpectra", false,
                  "Whether or not to remove spectra that have been masked from "
                  "the inputworkspace.",
                  Direction::Input);
  declareProperty(
      "RemoveSpectraWithNoDetector", false,
      "Whether or not to remove spectra that have no attached detector.",
      Direction::Input);
}

std::map<std::string, std::string> RemoveSpectra::validateInputs() {
  std::map<std::string, std::string> errors;
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const auto workspace2D =
      boost::dynamic_pointer_cast<const Workspace2D>(inputWS);
  const auto eventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (!workspace2D && !eventWS) {
    errors.insert(std::make_pair(
        "InputWorkspace",
        "A none-Workspace2D or EventWorkspace has been provided."));
  }

  const std::vector<size_t> indexList = getProperty("WorkspaceIndices");
  for (const auto &index : indexList) {
    // index >= 0 is assumed as it should be a const long unsinged int
    if (index > inputWS->getNumberHistograms()) {
      errors.insert(
          std::make_pair("WorkspaceIndices",
                         "Passed Workspace Index: " + std::to_string(index) +
                             " is not valid for passed InputWorkspace"));
    }
  }

  return errors;
}

namespace {
template <class T> bool evaluateSpectrumForDetectors(T spectrum) {
  const auto detectors = spectrum->getDetectorIDs();
  return detectors.empty();
}

std::vector<size_t>
discoverSpectraWithNoDetector(const MatrixWorkspace_sptr &inputWS) {
  std::vector<size_t> specIDs;
  for (auto i = 0u; i < inputWS->getNumberHistograms(); ++i) {
    const Histogram1D *spectrum1D =
        dynamic_cast<const Histogram1D *>(&inputWS->getSpectrum(i));
    const EventList *spectrumEvent =
        dynamic_cast<const EventList *>(&inputWS->getSpectrum(i));
    if (spectrumEvent &&
        evaluateSpectrumForDetectors<const EventList *>(spectrumEvent)) {

    } else if (spectrum1D &&
               evaluateSpectrumForDetectors<const Histogram1D *>(spectrum1D)) {
    }
  }
  return specIDs;
}

std::vector<size_t>
discoverSpectraWithMask(const MatrixWorkspace_sptr &inputWS) {
  std::vector<size_t> specIDs;
  for (auto i = 0u; i < inputWS->getNumberHistograms(); ++i) {
    if (inputWS->hasMaskedBins(i)) {
      specIDs.emplace_back(i);
    }
  }
  return specIDs;
}

template <class T>
bool evaluateIfSpectrumIsInList(std::vector<size_t> &specList, T spectrum) {
  const auto it =
      std::find(specList.begin(), specList.end(), spectrum->getSpectrumNo());
  return it != specList.end();
}

void removeDuplicates(std::vector<size_t> &specList) {
  // It is faster to default construct a set/unordered_set and insert the
  // elements than use the range-based constructor directly.
  // See https://stackoverflow.com/a/24477023:
  //   "the constructor actually constructs a new node for every element,
  //   before checking its value to determine if it should actually be
  //   inserted."
  std::unordered_set<size_t> uniqueList;
  for (const auto &spec : specList) {
    uniqueList.insert(spec);
  }
  specList.assign(std::begin(uniqueList), std::end(uniqueList));
}
} // namespace

void RemoveSpectra::exec() {
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string outputWorkspaceName = getPropertyValue("OutputWorkspace");
  std::vector<size_t> specList = getProperty("WorkspaceIndices");
  const bool removeMaskedSpectra = getProperty("RemoveMaskedSpectra");
  const bool removeSpectraWithNoDetector =
      getProperty("RemoveSpectraWithNoDetector");

  if (specList.empty() && removeMaskedSpectra && removeSpectraWithNoDetector) {
    g_log.warning("Nothing passed to the RemoveSpectra algorithm to remove so "
                  "nothing happened");
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
    return;
  }

  // Guarantee there are no duplicates in speclist - I don't know if this is
  // useful yet.
  removeDuplicates(specList);

  auto outputWS = copySpectraFromInputToOutput(inputWS, specList);

  g_log.debug(std::to_string(specList.size()) + " spectra removed.");

  AnalysisDataService::Instance().addOrReplace(outputWorkspaceName, outputWS);
  setProperty("OutputWorkspace", outputWS);
}

MatrixWorkspace_sptr RemoveSpectra::copySpectraFromInputToOutput(
    MatrixWorkspace_sptr inputWS, const std::vector<size_t> &specList) {
  std::vector<size_t> indicesToExtract;
  for (size_t i = 0; i < inputWS->getNumberHistograms(); ++i) {
    if (std::find(specList.begin(), specList.end(), i) == specList.end()) {
      indicesToExtract.emplace_back(i);
    }
  }

  if (indicesToExtract.empty()) {
    return MatrixWorkspace_sptr();
  }

  auto extractSpectra = createChildAlgorithm("ExtractSpectra");
  extractSpectra->setProperty("InputWorkspace", inputWS);
  extractSpectra->setProperty("WorkspaceIndexList", indicesToExtract);
  extractSpectra->executeAsChildAlg();

  MatrixWorkspace_sptr outputWS =
      extractSpectra->getProperty("OutputWorkspace");
  return outputWS;
}
} // namespace Algorithms
} // namespace Mantid