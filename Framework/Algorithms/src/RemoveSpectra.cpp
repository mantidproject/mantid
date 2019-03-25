// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/RemoveSpectra.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
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
      std::make_unique<ArrayProperty<int32_t>>("SpectrumList",
                                               Direction::Input),
      "A comma-separated list of individual spectra numbers to remove");
  declareProperty("RemoveMaskedSpectra", false,
                  "Whether or not to remove spectra that have been masked from "
                  "the inputworkspace.",
                  Direction::Input);
  declareProperty(
      "RemoveSpectraWithNoDetector", false,
      "Whether or not to remove spectra that have no attached detector.",
      Direction::Input);
  declareProperty("OverwriteExisting", true,
                  "If true any existing workspaces with the output name will be"
                  " overwritten.",
                  Direction::Input);
}

std::map<std::string, std::string> RemoveSpectra::validateInputs() {
  std::map<std::string, std::string> errors;
  bool overwriteExisting = getProperty("OverwriteExisting");
  std::string outputWorkspaceName = getPropertyValue("OutputWorkspace");
  if (AnalysisDataService::Instance().isValid(outputWorkspaceName) == "" &&
      !overwriteExisting) {
    errors.insert(std::make_pair("OverwriteExisting",
                                 "Cannot overwrite existing output workspace"));
  }

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const auto workspace2D =
      boost::dynamic_pointer_cast<const Workspace2D>(inputWS);
  const auto eventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (!workspace2D && !eventWS) {
    errors.insert(std::make_pair(
        "InputWorkspace",
        "A none-Workspace2D or EventWorkspace has been provided."));
  }

  // Ensure the specIDs passed are valid
  std::vector<int32_t> specList = getProperty("SpectrumList");
  std::vector<SpectrumNumber> spectrumNumbersInWS =
      inputWS->indexInfo().spectrumNumbers();
  for (const auto &specNum : specList) {
    const auto it = std::find(spectrumNumbersInWS.begin(),
                              spectrumNumbersInWS.end(), specNum);
    if (it == spectrumNumbersInWS.end()) {
      errors.insert(std::make_pair(
          "SpectrumList", "Spectrum number " + std::to_string(specNum) +
                              " does not exist in workspace."));
      break;
    }
  }
  return errors;
}

void RemoveSpectra::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const std::string outputWorkspaceName = getPropertyValue("OutputWorkspace");
  std::vector<int32_t> specList = getProperty("SpectrumList");
  const bool removeMaskedSpectra = getProperty("RemoveMaskedSpectra");
  const bool removeSpectraWithNoDetector =
      getProperty("RemoveSpectraWithNoDetector");
  const bool overwriteExisting = getProperty("OverwriteExisting");

  if (specList.empty() && removeMaskedSpectra && removeSpectraWithNoDetector) {
    g_log.warning("Nothing passed to the RemoveSpectra algorithm to remove so "
                  "nothing happened");
    return;
  }

  if (removeMaskedSpectra || removeSpectraWithNoDetector) {
    const auto extraSpectra = discoverSpectraWithNoDetectorOrisMasked(inputWS);
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

  if (overwriteExisting)
    AnalysisDataService::Instance().addOrReplace(outputWorkspaceName, outputWS);
  else
    AnalysisDataService::Instance().add(outputWorkspaceName, outputWS);

  setProperty("OutputWorkspace", outputWS);
}

std::vector<int32_t> RemoveSpectra::discoverSpectraWithNoDetectorOrisMasked(
    MatrixWorkspace_const_sptr &inputWS) {

  std::vector<int32_t> specIDs;

  for (auto i = 0u; i < inputWS->getNumberHistograms(); ++i) {
    const Histogram1D *spectrum1D =
        dynamic_cast<const Histogram1D *>(&inputWS->getSpectrum(i));
    const EventList *spectrumEvent =
        dynamic_cast<const EventList *>(&inputWS->getSpectrum(i));

    if (spectrumEvent && evaluateSpectrumForMaskOrDetectors<const EventList *>(
                             inputWS, spectrumEvent, i)) {
      specIDs.emplace_back(spectrumEvent->getSpectrumNo());
    } else if (spectrum1D &&
               evaluateSpectrumForMaskOrDetectors<const Histogram1D *>(
                   inputWS, spectrum1D, i)) {
      specIDs.emplace_back(spectrum1D->getSpectrumNo());
    }
  }

  return specIDs;
}

template <class T>
bool RemoveSpectra::evaluateSpectrumForMaskOrDetectors(
    MatrixWorkspace_const_sptr &inputWS, T spectrum, int index) {
  const auto detectors = spectrum->getDetectorIDs();
  bool returnValue = false;

  if (getProperty("RemoveSpectraWithNoDetector")) {
    returnValue = detectors.empty();
  }
  if (getProperty("RemoveMaskedSpectra") && !returnValue) {
    returnValue = inputWS->hasMaskedBins(index);
  }
  return returnValue;
}

MatrixWorkspace_sptr
RemoveSpectra::copySpectraFromInputToOutput(MatrixWorkspace_const_sptr &inputWS,
                                            std::vector<int32_t> &specList) {
  const auto numOfOutSpectrum =
      inputWS->getNumberHistograms() - specList.size();
  auto outputWS = DataObjects::create<MatrixWorkspace>(
      *inputWS.get(), numOfOutSpectrum, HistogramData::BinEdges(2));

  auto outputWSindex = 0;
  for (auto i = 0u; i < inputWS->getNumberHistograms(); ++i) {
    const Histogram1D *spectrum1D =
        dynamic_cast<const Histogram1D *>(&inputWS->getSpectrum(i));
    const EventList *spectrumEvent =
        dynamic_cast<const EventList *>(&inputWS->getSpectrum(i));

    if (spectrum1D && !evaluateIfSpectrumIsInList<const Histogram1D *>(
                          specList, spectrum1D)) {
      outputWS->getSpectrum(outputWSindex).copyDataFrom(*spectrum1D);
      outputWS->getSpectrum(outputWSindex)
          .setSpectrumNo(spectrum1D->getSpectrumNo());
      ++outputWSindex;
    } else if (spectrumEvent && !evaluateIfSpectrumIsInList<const EventList *>(
                                    specList, spectrumEvent)) {
      outputWS->getSpectrum(outputWSindex).copyDataFrom(*spectrumEvent);
      outputWS->getSpectrum(outputWSindex)
          .setSpectrumNo(spectrumEvent->getSpectrumNo());
      ++outputWSindex;
    }
  }

  return outputWS;
}

template <class T>
bool RemoveSpectra::evaluateIfSpectrumIsInList(std::vector<int32_t> &specList,
                                               T spectrum) {
  const auto it =
      std::find(specList.begin(), specList.end(), spectrum->getSpectrumNo());
  return it != specList.end();
}

void RemoveSpectra::removeDuplicates(std::vector<int32_t> &specList) {
  // It is faster to default construct a set/unordered_set and insert the
  // elements than use the range-based constructor directly.
  // See https://stackoverflow.com/a/24477023:
  //   "the constructor actually construct a new node for every element, before
  //   checking its value to determine if it should actually be inserted."
  std::unordered_set<int32_t> uniqueList;
  for (const auto &spec : specList) {
    uniqueList.insert(spec);
  }
  specList.assign(std::begin(uniqueList), std::end(uniqueList));
}

} // namespace Algorithms
} // namespace Mantid