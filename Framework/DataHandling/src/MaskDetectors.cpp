// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/MaskDetectors.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"

#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/LegacyConversion.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <algorithm>
#include <numeric>
#include <set>

namespace { // declare file scoped function
/** internal method copies values in specified range from source list to the
target list
@param sourceList :: vector of input values
@param targetList :: vector of output values
@param minIndex   :: min value to include in the list of output values
@param maxIndex   :: max value to include in the list of output values
*/
void constrainIndexInRange(std::vector<size_t> &sourceList, std::vector<size_t> &targetList, size_t minIndex,
                           size_t maxIndex) {
  targetList.reserve(sourceList.size());
  std::sort(sourceList.begin(), sourceList.end());
  std::copy_if(sourceList.cbegin(), sourceList.cend(), std::back_inserter(targetList),
               [minIndex, maxIndex](auto sourceValue) { return sourceValue >= minIndex && sourceValue <= maxIndex; });
}
} // namespace

namespace Mantid::DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MaskDetectors)

using namespace API;
using namespace Kernel;
using namespace DataObjects;
using namespace Geometry;

/*
 * Define input arguments
 */
void MaskDetectors::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "", Direction::InOut),
                  "The name of the input and output workspace on which to perform the "
                  "algorithm.");
  declareProperty(std::make_unique<ArrayProperty<specnum_t>>("SpectraList"), "A list of spectra to mask");
  declareProperty(std::make_unique<ArrayProperty<detid_t>>("DetectorList"), "A list of detector ID's to mask");
  declareProperty(std::make_unique<ArrayProperty<size_t>>("WorkspaceIndexList"),
                  "A list of the workspace indices to mask");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("MaskedWorkspace", "", Direction::Input, PropertyMode::Optional),
      "If given but not as a SpecialWorkspace2D, the masking from "
      "this workspace will be copied. If given as a "
      "SpecialWorkspace2D, the masking is read from its Y values.");
  declareProperty("ForceInstrumentMasking", false,
                  "Works when 'MaskedWorkspace' is provided and forces "
                  "to use spectra-detector mapping even in case when number of "
                  "spectra in 'Workspace' and 'MaskedWorkspace' are equal",
                  Direction::Input);
  setPropertySettings("ForceInstrumentMasking",
                      std::make_unique<EnabledWhenProperty>("MaskedWorkspace", ePropertyCriterion::IS_NOT_DEFAULT));

  auto mustBePosInt = std::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
                  "If other masks fields are provided, it's the first index of the "
                  "target workspace to be allowed to be masked from by these masks, "
                  "if not, its the first index of the target workspace to mask.\n"
                  "Default value is 0 if other masking is present or ignored if not.");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePosInt,
                  "If other masks are provided, it's the last index of the "
                  "target workspace allowed to be masked to by these masks, "
                  "if not, its the last index of the target workspace to mask.\n"
                  "Default is number of histograms in target workspace if other masks are"
                  " present "
                  "or ignored if not.");
  declareProperty(std::make_unique<ArrayProperty<std::string>>("ComponentList"), "A list names of components to mask");
}

/*
 * Main exec body
 */
void MaskDetectors::exec() {
  // Get the input workspace
  Workspace_sptr propWS = getProperty("Workspace");
  MatrixWorkspace_sptr WS = std::dynamic_pointer_cast<MatrixWorkspace>(propWS);
  PeaksWorkspace_sptr peaksWS = std::dynamic_pointer_cast<PeaksWorkspace>(propWS);
  if (peaksWS) {
    execPeaks(peaksWS);
    return;
  }

  // Is it an event workspace?
  EventWorkspace_sptr eventWS = std::dynamic_pointer_cast<EventWorkspace>(WS);

  // Is it a Mask Workspace ?
  MaskWorkspace_sptr inputAsMaskWS = std::dynamic_pointer_cast<MaskWorkspace>(WS);
  const auto isMaskWS = static_cast<bool>(inputAsMaskWS);

  std::vector<size_t> indexList = getProperty("WorkspaceIndexList");
  auto spectraList = Indexing::makeSpectrumNumberVector(getProperty("SpectraList"));
  std::vector<detid_t> detectorList = getProperty("DetectorList");
  std::vector<std::string> componentList = getProperty("ComponentList");
  if (!componentList.empty()) {
    appendToDetectorListFromComponentList(detectorList, componentList, WS);
  }
  const MatrixWorkspace_sptr prevMasking = getProperty("MaskedWorkspace");

  auto ranges_info = getRanges(WS);
  bool range_constrained = std::get<2>(ranges_info);

  bool mask_defined(false);
  if (!indexList.empty() || !spectraList.empty() || !detectorList.empty() || prevMasking) {
    mask_defined = true;
  }

  // each one of these values is optional but the user can not leave all six
  // blank
  if (!mask_defined && !range_constrained) {
    g_log.information(name() + ": There is nothing to mask, the index, spectra, "
                               "detector lists and masked workspace properties are all empty");
    return;
  }

  // Index range are provided as min/max values
  if (!mask_defined && range_constrained) {
    size_t list_size = std::get<1>(ranges_info) - std::get<0>(ranges_info) + 1;
    indexList.resize(list_size);
    std::iota(indexList.begin(), indexList.end(), std::get<0>(ranges_info));
  }

  auto maskWS = std::dynamic_pointer_cast<DataObjects::MaskWorkspace>(prevMasking);
  if (maskWS && prevMasking) { // is a mask workspace
    handleMaskByMaskWorkspace(maskWS, WS, detectorList, indexList, ranges_info);
  } else if (prevMasking) { // is not a mask workspace
    handleMaskByMatrixWorkspace(prevMasking, WS, detectorList, indexList, ranges_info);
  }

  // If the spectraList property has been set, need to loop over the workspace
  // looking for the
  // appropriate spectra number and adding the indices they are linked to the
  // list to be processed
  if (!spectraList.empty()) {
    fillIndexListFromSpectra(indexList, std::move(spectraList), WS, ranges_info);
  } // End dealing with spectraList
  if (!detectorList.empty()) {
    // Convert from detectors to workspace indexes
    if (indexList.empty()) {
      indexList = WS->getIndicesFromDetectorIDs(detectorList);
    } else {
      auto tmpList = WS->getIndicesFromDetectorIDs(detectorList);
      indexList.insert(indexList.end(), std::begin(tmpList), std::end(tmpList));
    }
    detectorList.clear();
    //
    // Constrain by ws indexes provided, if any
    if (range_constrained) {
      constrainMaskedIndexes(indexList, ranges_info);
    }
  }

  if (indexList.empty()) {
    g_log.warning("No spectra affected.");
    return;
  }

  // Get a reference to the spectra-detector map to get hold of detector ID's
  auto &spectrumInfo = WS->mutableSpectrumInfo();
  double prog = 0.0;
  for (const auto i : indexList) {
    WS->getSpectrum(i).clearData();
    if (spectrumInfo.hasDetectors(i))
      spectrumInfo.setMasked(i, true);

    // Progress
    prog += (1.0 / static_cast<int>(indexList.size()));
    progress(prog);
  }

  if (eventWS) {
    // Also clear the MRU for event workspaces.
    eventWS->clearMRU();
  }

  if (isMaskWS) {
    // When input is a MaskWorkspace, some special handling is needed.
    auto &maskWsSpectrumInfo = inputAsMaskWS->mutableSpectrumInfo();
    for (size_t i = 0; i < inputAsMaskWS->getNumberHistograms(); ++i) {
      const bool mask = inputAsMaskWS->isMaskedIndex(i) || maskWsSpectrumInfo.isMasked(i);
      inputAsMaskWS->setMaskedIndex(i, mask);
      // Always clear the mask flag from MaskWorkspace
      maskWsSpectrumInfo.setMasked(i, false);
    }
  }
}

/** Handle masking a workspace using a MaskWorkspace.
 *
 * This will choose the strategy to mask the input workspace with based on
 * information about the mask workspace.
 *
 * @param maskWs :: pointer to the mask workspace to use.
 * @param WS :: pointer to the workspace to be masked.
 * @param detectorList :: list of detectors to be masked.
 * @param indexList :: list of workspace indicies to be masked.
 * @param rangeInfo :: information about the spectrum range to use when masking
 */
void MaskDetectors::handleMaskByMaskWorkspace(const MaskWorkspace_const_sptr &maskWs,
                                              const API::MatrixWorkspace_const_sptr &WS,
                                              std::vector<detid_t> &detectorList, std::vector<size_t> &indexList,
                                              const RangeInfo &rangeInfo) {

  if (maskWs->getInstrument()->getDetectorIDs().size() != WS->getInstrument()->getDetectorIDs().size()) {
    throw std::runtime_error("Instrument's detector numbers mismatch "
                             "between input Workspace and MaskWorkspace");
  }

  g_log.debug() << "Extracting mask from MaskWorkspace (" << maskWs->getName() << ")\n";
  bool forceDetIDs = getProperty("ForceInstrumentMasking");
  if (maskWs->getNumberHistograms() != WS->getNumberHistograms() || forceDetIDs) {
    g_log.notice("Masking using detectors IDs");
    extractMaskedWSDetIDs(detectorList, maskWs);
  } else {
    g_log.notice("Masking using workspace indicies");
    appendToIndexListFromMaskWS(indexList, maskWs, rangeInfo);
  }
}

/** Handle masking a workspace using a MatrixWorkspace.
 *
 * This will choose the strategy to mask the input workspace with based on
 * information about the matrix workspace passed as a mask workspace.
 *
 * @param maskWs :: pointer to the workspace to use as a mask.
 * @param WS :: pointer to the workspace to be masked.
 * @param detectorList :: list of detectors to be masked.
 * @param indexList :: list of workspace indicies to be masked.
 * @param rangeInfo :: information about the spectrum range to use when masking
 */
void MaskDetectors::handleMaskByMatrixWorkspace(const API::MatrixWorkspace_const_sptr &maskWs,
                                                const API::MatrixWorkspace_const_sptr &WS,
                                                std::vector<detid_t> &detectorList, std::vector<size_t> &indexList,
                                                const RangeInfo &rangeInfo) {

  const auto nHist = maskWs->getNumberHistograms();
  auto instrument = WS->getInstrument();
  auto maskInstrument = maskWs->getInstrument();

  // Check the provided workspace has the same number of spectra as the
  // input, if so assume index list
  if (nHist == WS->getNumberHistograms()) {
    g_log.notice("Masking using workspace indicies");
    appendToIndexListFromWS(indexList, maskWs, rangeInfo);
    // Check they both have instrument and then use detector based masking
  } else if (instrument && maskInstrument) {
    const auto inputDetCount = instrument->getNumberDetectors();
    const auto maskDetCount = maskInstrument->getNumberDetectors();

    g_log.notice("Masking using detectors IDs");

    if (inputDetCount != maskDetCount)
      g_log.warning() << "Number of detectors does not match between "
                         "mask workspace and input workspace";

    if (instrument->getName() != maskInstrument->getName())
      g_log.warning() << "Mask is from a different instrument to the input "
                         "workspace. ";

    appendToDetectorListFromWS(detectorList, WS, maskWs, rangeInfo);
    // Not an index list and there's no instrument, so give up.
  } else {
    throw std::runtime_error("Input or mask workspace does not have an instrument.");
  }
}

/* Verifies input ranges are defined and returns these ranges if they are.
 *
 * @return tuple containing min/max ranges provided to algorithm
 *        (from 0 to max histogram range) and Boolean value,
 *        containing true if the ranges are
 *        constrained (or defined in other words)
 *        and false if they are not.
 */
std::tuple<size_t, size_t, bool> MaskDetectors::getRanges(const MatrixWorkspace_sptr &targWS) {
  int endIndex = getProperty("EndWorkspaceIndex");
  int startIndex = getProperty("StartWorkspaceIndex");
  size_t max_ind = targWS->getNumberHistograms() - 1;

  if (endIndex == EMPTY_INT() && startIndex == 0) {
    return std::tuple<size_t, size_t, bool>(0, max_ind, false);
  } else {
    if (startIndex < 0) {
      startIndex = 0;
    }
    auto startIndex_l = static_cast<size_t>(startIndex);
    auto endIndex_l = static_cast<size_t>(endIndex);

    if (endIndex == EMPTY_INT()) {
      endIndex_l = max_ind;
    }
    if (endIndex_l > max_ind) {
      endIndex_l = max_ind;
    }
    if (startIndex_l > endIndex_l) {
      startIndex_l = endIndex_l;
    }

    return std::tuple<size_t, size_t, bool>(startIndex_l, endIndex_l, true);
  }
}
/* Do constrain masked indexes by limits, provided as input
 * @param indexList  :: list of indexes to verify against constrain on input
 *                      and list of constrained indexes on the output.
 * @param startIndex :: minimal index (inclusive) to include in the constrained
 *                      list.
 * @param endIndex   :: maximal index (inclusive) to include in the constrained
 *                      list.
 */
void MaskDetectors::constrainMaskedIndexes(std::vector<size_t> &indexList,
                                           const std::tuple<size_t, size_t, bool> &range_info) {

  std::vector<size_t> tmp;
  constrainIndexInRange(indexList, tmp, std::get<0>(range_info), std::get<1>(range_info));
  tmp.swap(indexList);
}

/* Method to extract detector's id-s from mask workspace
* the mask workspace assumed to be not having masked detectors, but has masked
* state defined in its spectra
@param detectorList :: list of masked detectors, appended on output by the
*                      detectors, defined in the mask workspace.
@param maskWS       :: shared pointer to workspace containing masks.
*/
void MaskDetectors::extractMaskedWSDetIDs(std::vector<detid_t> &detectorList,
                                          const DataObjects::MaskWorkspace_const_sptr &maskWS) {

  size_t nHist = maskWS->getNumberHistograms();
  for (size_t i = 0; i < nHist; ++i) {
    if (maskWS->y(i).front() > 0.5) {
      const auto &dets = maskWS->getSpectrum(i).getDetectorIDs();
      std::copy(dets.cbegin(), dets.cend(), std::back_inserter(detectorList));
    }
  }
}

/*
 * Peaks exec body
 * @param WS :: The input peaks workspace to be masked
 */
void MaskDetectors::execPeaks(const PeaksWorkspace_sptr &WS) {
  std::vector<detid_t> detectorList = getProperty("DetectorList");
  const MatrixWorkspace_sptr prevMasking = getProperty("MaskedWorkspace");

  // each one of these values is optional but the user can't leave all four
  // blank
  if (detectorList.empty() && !prevMasking) {
    g_log.information(name() + ": There is nothing to mask, "
                               "detector lists and masked workspace properties are all empty");
    return;
  }

  auto &detInfo = WS->mutableDetectorInfo();
  std::vector<size_t> indicesToMask;
  for (const auto &detID : detectorList) {
    try {
      indicesToMask.emplace_back(detInfo.indexOf(detID));
    } catch (std::out_of_range &) {
      g_log.warning() << "Invalid detector ID " << detID << ". Found while running MaskDetectors\n";
    }
  }

  // If we have a workspace that could contain masking,copy that in too
  if (prevMasking) {
    DataObjects::MaskWorkspace_sptr maskWS = std::dynamic_pointer_cast<DataObjects::MaskWorkspace>(prevMasking);
    if (maskWS) {
      const auto &maskDetInfo = maskWS->detectorInfo();
      if (detInfo.size() != maskDetInfo.size()) {
        throw std::runtime_error("Size mismatch between input Workspace and MaskWorkspace");
      }

      g_log.debug() << "Extracting mask from MaskWorkspace (" << maskWS->getName() << ")\n";

      for (size_t i = 0; i < maskDetInfo.size(); ++i)
        if (maskDetInfo.isMasked(i))
          indicesToMask.emplace_back(i);
    }
  }

  for (const auto index : indicesToMask)
    detInfo.setMasked(index, true);
}

/* Convert a list of spectra numbers into the corresponding workspace indices.
 *
 * @param indexList :: An output index list from the given spectra list
 * @param spectraList :: A list of spectra numbers
 * @param WS :: The input workspace to be masked
 * @param range_info :: tuple containing the range of spectra to process and
 *                      Boolean indicating if these ranges are defined
 */
void MaskDetectors::fillIndexListFromSpectra(std::vector<size_t> &indexList,
                                             std::vector<Indexing::SpectrumNumber> spectraList,
                                             const API::MatrixWorkspace_sptr &WS,
                                             const std::tuple<size_t, size_t, bool> &range_info) {

  std::vector<size_t> tmp_index;
  size_t startIndex = std::get<0>(range_info);
  size_t endIndex = std::get<1>(range_info);
  bool range_constrained = std::get<2>(range_info);

  if (range_constrained) {
    constrainIndexInRange(indexList, tmp_index, startIndex, endIndex);
  } else {
    tmp_index.swap(indexList);
  }

  // Ignore duplicate entries.
  std::sort(spectraList.begin(), spectraList.end());
  auto last = std::unique(spectraList.begin(), spectraList.end());
  if (last != spectraList.end())
    g_log.warning("Duplicate entries in spectrum list.");
  spectraList.erase(last, spectraList.end());
  for (auto ws_index : WS->indexInfo().makeIndexSet(spectraList)) {
    if (range_constrained && (ws_index < startIndex || ws_index > endIndex)) {
      continue;
    }
    tmp_index.emplace_back(ws_index);
  }

  tmp_index.swap(indexList);
}

/* Append the indices of the masked spectra from the given workspace list to the
 * given list
 *
 * @param indexList :: An existing list of indices.
 * @param sourceWS  :: An workspace with masked spectra.
 * @param range_info :: tuple containing the range of spectra to process and
 *                            Boolean indicating if these ranges are defined
 */
void MaskDetectors::appendToIndexListFromWS(std::vector<size_t> &indexList, const MatrixWorkspace_const_sptr &sourceWS,
                                            const std::tuple<size_t, size_t, bool> &range_info) {

  std::vector<size_t> tmp_index;
  size_t startIndex = std::get<0>(range_info);
  size_t endIndex = std::get<1>(range_info);
  bool range_constrained = std::get<2>(range_info);

  const auto &spectrumInfo = sourceWS->spectrumInfo();
  if (range_constrained) {
    constrainIndexInRange(indexList, tmp_index, startIndex, endIndex);

    for (size_t i = startIndex; i <= endIndex; ++i) {
      if (spectrumInfo.hasDetectors(i) && spectrumInfo.isMasked(i)) {
        tmp_index.emplace_back(i);
      }
    }
  } else {
    tmp_index.swap(indexList);

    endIndex = sourceWS->getNumberHistograms();
    for (size_t i = 0; i < endIndex; ++i) {
      if (spectrumInfo.hasDetectors(i) && spectrumInfo.isMasked(i)) {
        tmp_index.emplace_back(i);
      }
    }
  }
  tmp_index.swap(indexList);
}

/**
 * Append the indices of a workspace corresponding to detector IDs to the
 * given list
 *
 * @param detectorList :: An existing list of detector IDs
 * @param inputWs :: A workspace to mask detectors in
 * @param maskWs :: A workspace with masked spectra
 * @param range_info    :: tuple containing the information on
 *                      if copied indexes are constrained by ranges and if yes
 *                       -- the range of indexes to topy
 */
void MaskDetectors::appendToDetectorListFromWS(std::vector<detid_t> &detectorList,
                                               const MatrixWorkspace_const_sptr &inputWs,
                                               const MatrixWorkspace_const_sptr &maskWs,
                                               const std::tuple<size_t, size_t, bool> &range_info) {
  const auto startIndex = std::get<0>(range_info);
  const auto endIndex = std::get<1>(range_info);
  const auto &detMap = inputWs->getDetectorIDToWorkspaceIndexMap();
  detectorList.reserve(maskWs->getNumberHistograms());

  for (size_t i = 0; i < maskWs->getNumberHistograms(); ++i) {
    if (maskWs->y(i)[0] == 0) {
      const auto &detIds = maskWs->getSpectrum(i).getDetectorIDs();
      auto condition = [detMap, startIndex, endIndex](const auto id) {
        return detMap.at(id) >= startIndex && detMap.at(id) <= endIndex;
      };
      std::copy_if(detIds.cbegin(), detIds.cend(), std::back_inserter(detectorList), condition);
    }
  }
}

/**
 * Append the indices of the masked spectra from the given workspace list to the
 * given list
 * @param indexList :: An existing list of indices
 * @param maskedWorkspace :: An workspace with masked spectra
 * @param range_info    :: tuple containing the information on
 *                      if copied indexes are constrained by ranges and if yes
 *                       -- the range of indexes to topy
 */
void MaskDetectors::appendToIndexListFromMaskWS(std::vector<size_t> &indexList,
                                                const DataObjects::MaskWorkspace_const_sptr &maskedWorkspace,
                                                const std::tuple<size_t, size_t, bool> &range_info) {

  std::vector<size_t> tmp_index;

  size_t startIndex = std::get<0>(range_info);
  size_t endIndex = std::get<1>(range_info);
  bool range_constrained = std::get<2>(range_info);

  if (range_constrained) {
    constrainIndexInRange(indexList, tmp_index, startIndex, endIndex);

    for (size_t i = startIndex; i <= endIndex; ++i) {
      if (maskedWorkspace->y(i)[0] > 0.5) {
        g_log.debug() << "Adding WorkspaceIndex " << i << " to mask.\n";
        tmp_index.emplace_back(i);
      }
    }
  } else {
    tmp_index.swap(indexList);
    endIndex = maskedWorkspace->getNumberHistograms();
    for (size_t i = 0; i < endIndex; ++i) {

      if (maskedWorkspace->y(i)[0] > 0.5) {
        g_log.debug() << "Adding WorkspaceIndex " << i << " to mask.\n";
        tmp_index.emplace_back(i);
      }
    }
  }
  tmp_index.swap(indexList);
} // appendToIndexListFromWS

/**
 * Append the detector IDs of detectors found recursively in the list of
 * components.
 *
 * @param detectorList :: An existing list of detector IDs
 * @param componentList :: List of component names
 * @param WS :: Workspace instrument of which to use
 */
void MaskDetectors::appendToDetectorListFromComponentList(std::vector<detid_t> &detectorList,
                                                          const std::vector<std::string> &componentList,
                                                          const API::MatrixWorkspace_const_sptr &WS) {
  const auto instrument = WS->getInstrument();
  if (!instrument) {
    g_log.error() << "No instrument in input workspace. Ignoring ComponentList\n";
    return;
  }
  std::set<detid_t> detectorIDs;
  for (const auto &compName : componentList) {
    std::vector<IDetector_const_sptr> dets;
    instrument->getDetectorsInBank(dets, compName);
    if (dets.empty()) {
      const auto component = instrument->getComponentByName(compName);
      const auto det = std::dynamic_pointer_cast<const IDetector>(component);
      if (!det) {
        g_log.warning() << "No detectors found in component '" << compName << "'\n";
        continue;
      }
      dets.emplace_back(det);
    }
    for (const auto &det : dets) {
      detectorIDs.emplace(det->getID());
    }
  }
  const auto oldSize = detectorList.size();
  detectorList.resize(detectorList.size() + detectorIDs.size());
  auto appendBegin = detectorList.begin() + oldSize;
  std::copy(detectorIDs.cbegin(), detectorIDs.cend(), appendBegin);
} // appendToDetectorListFromComponentList

} // namespace Mantid::DataHandling
