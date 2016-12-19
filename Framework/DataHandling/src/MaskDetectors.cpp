#include "MantidDataHandling/MaskDetectors.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <set>
#include <numeric>

namespace { // declare file scoped function
/** internal method copies values in specified range from source list to the
target list
@param sourceList :: vector of input values
@param targetList :: vector of output values
@param minIndex   :: min value to include in the list of output values
@param maxIndex   :: max value to include in the list of output values
*/
void constrainIndexInRange(std::vector<size_t> &sourceList,
                           std::vector<size_t> &targetList, size_t minIndex,
                           size_t maxIndex) {
  targetList.reserve(sourceList.size());
  std::sort(sourceList.begin(), sourceList.end());
  for (auto memb : sourceList) {
    if (memb >= minIndex && memb <= maxIndex) {
      targetList.push_back(memb);
    }
  }
}
}

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MaskDetectors)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using Geometry::Instrument_const_sptr;
using Geometry::IDetector_const_sptr;
using namespace DataObjects;

/*
 * Define input arguments
 */
void MaskDetectors::init() {
  declareProperty(
      make_unique<WorkspaceProperty<Workspace>>("Workspace", "",
                                                Direction::InOut),
      "The name of the input and output workspace on which to perform the "
      "algorithm.");
  declareProperty(make_unique<ArrayProperty<specnum_t>>("SpectraList"),
                  "An ArrayProperty containing a list of spectra to mask");
  declareProperty(
      make_unique<ArrayProperty<detid_t>>("DetectorList"),
      "An ArrayProperty containing a list of detector ID's to mask");
  declareProperty(make_unique<ArrayProperty<size_t>>("WorkspaceIndexList"),
                  "An ArrayProperty containing the workspace indices to mask");
  declareProperty(make_unique<WorkspaceProperty<>>("MaskedWorkspace", "",
                                                   Direction::Input,
                                                   PropertyMode::Optional),
                  "If given but not as a SpecialWorkspace2D, the masking from "
                  "this workspace will be copied. If given as a "
                  "SpecialWorkspace2D, the masking is read from its Y values.");
  declareProperty("ForceInstrumentMasking", false,
                  "Works when 'MaskedWorkspace' is provided and forces "
                  "to use spectra-detector mapping even in case when number of "
                  "spectra in 'Workspace' and 'MaskedWorkspace' are equal",
                  Direction::Input);
  setPropertySettings(
      "ForceInstrumentMasking",
      make_unique<EnabledWhenProperty>("MaskedWorkspace",
                                       ePropertyCriterion::IS_NOT_DEFAULT));

  auto mustBePosInt = boost::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  declareProperty(
      "StartWorkspaceIndex", 0, mustBePosInt,
      "If other masks fields are provided, it's the first index of the "
      "target workspace to be allowed to be masked from by these masks, "
      "if not, its the first index of the target workspace to mask.\n"
      "Default value is 0 if other masking is present or ignored if not.");
  declareProperty(
      "EndWorkspaceIndex", EMPTY_INT(), mustBePosInt,
      "If other masks are provided, it's the last index of the "
      "target workspace allowed to be masked to by these masks, "
      "if not, its the last index of the target workspace to mask.\n"
      "Default is number of histograms in target workspace if other masks are"
      " present "
      "or ignored if not.");
}

/*
 * Main exec body
 */
void MaskDetectors::exec() {
  // Get the input workspace
  Workspace_sptr propWS = getProperty("Workspace");
  MatrixWorkspace_sptr WS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(propWS);
  PeaksWorkspace_sptr peaksWS =
      boost::dynamic_pointer_cast<PeaksWorkspace>(propWS);
  if (peaksWS) {
    execPeaks(peaksWS);
    return;
  }

  // Is it an event workspace?
  EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(WS);

  // Is it a Mask Workspace ?
  MaskWorkspace_sptr isMaskWS = boost::dynamic_pointer_cast<MaskWorkspace>(WS);

  std::vector<size_t> indexList = getProperty("WorkspaceIndexList");
  std::vector<specnum_t> spectraList = getProperty("SpectraList");
  std::vector<detid_t> detectorList = getProperty("DetectorList");
  const MatrixWorkspace_sptr prevMasking = getProperty("MaskedWorkspace");

  auto ranges_info = getRanges(WS);
  bool range_constrained = std::get<2>(ranges_info);

  bool mask_defined(false);
  if (!indexList.empty() || !spectraList.empty() || !detectorList.empty() ||
      prevMasking) {
    mask_defined = true;
  }

  // each one of these values is optional but the user can not leave all six
  // blank
  if (!mask_defined && !range_constrained) {
    g_log.information(
        name() +
        ": There is nothing to mask, the index, spectra, "
        "detector lists and masked workspace properties are all empty");
    return;
  }

  // Index range are provided as min/max values
  if (!mask_defined && range_constrained) {
    size_t list_size = std::get<1>(ranges_info) - std::get<0>(ranges_info) + 1;
    indexList.resize(list_size);
    std::iota(indexList.begin(), indexList.end(), std::get<0>(ranges_info));
  }

  if (prevMasking) {
    DataObjects::MaskWorkspace_const_sptr maskWS =
        boost::dynamic_pointer_cast<DataObjects::MaskWorkspace>(prevMasking);
    if (maskWS) {
      if (maskWS->getInstrument()->getDetectorIDs().size() !=
          WS->getInstrument()->getDetectorIDs().size()) {
        throw std::runtime_error("Instrument's detector numbers mismatch "
                                 "between input Workspace and MaskWorkspace");
      }

      g_log.debug() << "Extracting mask from MaskWorkspace (" << maskWS->name()
                    << ")\n";
      bool forceDetIDs = getProperty("ForceInstrumentMasking");
      if (prevMasking->getNumberHistograms() != WS->getNumberHistograms() ||
          forceDetIDs) {
        extractMaskedWSDetIDs(detectorList, maskWS);
      } else {
        appendToIndexListFromMaskWS(indexList, maskWS, ranges_info);
      }
    } else { // not mask workspace
      // Check the provided workspace has the same number of spectra as the
      // input
      if (prevMasking->getNumberHistograms() > WS->getNumberHistograms()) {
        g_log.error() << "Input workspace has " << WS->getNumberHistograms()
                      << " histograms   vs. "
                      << "Input masking workspace has "
                      << prevMasking->getNumberHistograms()
                      << " histograms. \n";
        throw std::runtime_error("Size mismatch between two input workspaces.");
      }
      appendToIndexListFromWS(indexList, prevMasking, ranges_info);
    }
  }

  // If the spectraList property has been set, need to loop over the workspace
  // looking for the
  // appropriate spectra number and adding the indices they are linked to the
  // list to be processed
  if (!spectraList.empty()) {
    fillIndexListFromSpectra(indexList, spectraList, WS, ranges_info);
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
    // If the input was a mask workspace, then extract the mask to ensure
    // we are returning the correct thing.
    IAlgorithm_sptr alg = createChildAlgorithm("ExtractMask");
    alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
    alg->executeAsChildAlg();
    MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
    setProperty("Workspace", ws);
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
std::tuple<size_t, size_t, bool>
MaskDetectors::getRanges(const MatrixWorkspace_sptr &targWS) {
  int endIndex = getProperty("EndWorkspaceIndex");
  int startIndex = getProperty("StartWorkspaceIndex");
  size_t max_ind = targWS->getNumberHistograms() - 1;

  if (endIndex == EMPTY_INT() && startIndex == 0) {
    return std::tuple<size_t, size_t, bool>(0, max_ind, false);
  } else {
    if (startIndex < 0) {
      startIndex = 0;
    }
    size_t startIndex_l = static_cast<size_t>(startIndex);
    size_t endIndex_l = static_cast<size_t>(endIndex);

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
void MaskDetectors::constrainMaskedIndexes(
    std::vector<size_t> &indexList,
    const std::tuple<size_t, size_t, bool> &range_info) {

  std::vector<size_t> tmp;
  constrainIndexInRange(indexList, tmp, std::get<0>(range_info),
                        std::get<1>(range_info));
  tmp.swap(indexList);
}

/* Method to extract detector's id-s from mask workspace
* the mask workspace assumed to be not having masked detectors, but has masked
* state defined in its spectra
@param detectorList :: list of masked detectors, appended on output by the
*                      detectors, defined in the mask workspace.
@param maskWS       :: shared pointer to workspace containing masks.
*/
void MaskDetectors::extractMaskedWSDetIDs(
    std::vector<detid_t> &detectorList,
    const DataObjects::MaskWorkspace_const_sptr &maskWS) {

  int64_t nHist = maskWS->getNumberHistograms();
  for (int64_t i = 0; i < nHist; ++i) {
    if (maskWS->readY(i)[0] > 0.5) {

      try {
        const auto dets = maskWS->getSpectrum(i).getDetectorIDs();
        for (auto det : dets) {
          detectorList.push_back(det);
        }
      } catch (Exception::NotFoundError &) {
        continue;
      }
    }
  }
}

/*
 * Peaks exec body
 * @param WS :: The input peaks workspace to be masked
 */
void MaskDetectors::execPeaks(PeaksWorkspace_sptr WS) {
  std::vector<detid_t> detectorList = getProperty("DetectorList");
  const MatrixWorkspace_sptr prevMasking = getProperty("MaskedWorkspace");

  // each one of these values is optional but the user can't leave all four
  // blank
  if (detectorList.empty() && !prevMasking) {
    g_log.information(
        name() +
        ": There is nothing to mask, "
        "detector lists and masked workspace properties are all empty");
    return;
  }

  auto &detInfo = WS->mutableDetectorInfo();
  std::vector<size_t> indicesToMask;
  for (const auto &detID : detectorList) {
    try {
      indicesToMask.push_back(detInfo.indexOf(detID));
    } catch (std::out_of_range &) {
      g_log.warning() << "Invalid detector ID " << detID
                      << ". Found while running MaskDetectors\n";
    }
  }

  // If we have a workspace that could contain masking,copy that in too
  if (prevMasking) {
    DataObjects::MaskWorkspace_sptr maskWS =
        boost::dynamic_pointer_cast<DataObjects::MaskWorkspace>(prevMasking);
    if (maskWS) {
      const auto &maskDetInfo = maskWS->detectorInfo();
      if (detInfo.size() != maskDetInfo.size()) {
        throw std::runtime_error(
            "Size mismatch between input Workspace and MaskWorkspace");
      }

      g_log.debug() << "Extracting mask from MaskWorkspace (" << maskWS->name()
                    << ")\n";

      for (size_t i = 0; i < maskDetInfo.size(); ++i)
        if (maskDetInfo.isMasked(i))
          indicesToMask.push_back(i);
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
void MaskDetectors::fillIndexListFromSpectra(
    std::vector<size_t> &indexList, const std::vector<specnum_t> &spectraList,
    const API::MatrixWorkspace_sptr WS,
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

  auto SpecID2IndMap = WS->getSpectrumToWorkspaceIndexMap();
  for (auto specnum : spectraList) {
    auto element = SpecID2IndMap.find(specnum);
    if (element == SpecID2IndMap.end()) {
      continue;
    }
    size_t ws_index = element->second;

    if (range_constrained && (ws_index < startIndex || ws_index > endIndex)) {
      continue;
    }
    tmp_index.push_back(ws_index);
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
void MaskDetectors::appendToIndexListFromWS(
    std::vector<size_t> &indexList, const MatrixWorkspace_sptr sourceWS,
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
        tmp_index.push_back(i);
      }
    }
  } else {
    tmp_index.swap(indexList);

    endIndex = sourceWS->getNumberHistograms();
    for (size_t i = 0; i < endIndex; ++i) {
      if (spectrumInfo.hasDetectors(i) && spectrumInfo.isMasked(i)) {
        tmp_index.push_back(i);
      }
    }
  }
  tmp_index.swap(indexList);

} // appendToIndexListFromWS

/**
* Append the indices of the masked spectra from the given workspace list to the
* given list
* @param indexList :: An existing list of indices
* @param maskedWorkspace :: An workspace with masked spectra
* @param range_info    :: tuple containing the information on
*                      if copied indexes are constrained by ranges and if yes
*                       -- the range of indexes to topy
*/
void MaskDetectors::appendToIndexListFromMaskWS(
    std::vector<size_t> &indexList,
    const DataObjects::MaskWorkspace_const_sptr maskedWorkspace,
    const std::tuple<size_t, size_t, bool> &range_info) {

  std::vector<size_t> tmp_index;

  size_t startIndex = std::get<0>(range_info);
  size_t endIndex = std::get<1>(range_info);
  bool range_constrained = std::get<2>(range_info);

  if (range_constrained) {
    constrainIndexInRange(indexList, tmp_index, startIndex, endIndex);

    for (size_t i = startIndex; i <= endIndex; ++i) {
      if (maskedWorkspace->dataY(i)[0] > 0.5) {
        g_log.debug() << "Adding WorkspaceIndex " << i << " to mask.\n";
        tmp_index.push_back(i);
      }
    }
  } else {
    tmp_index.swap(indexList);
    endIndex = maskedWorkspace->getNumberHistograms();
    for (size_t i = 0; i < endIndex; ++i) {

      if (maskedWorkspace->dataY(i)[0] > 0.5) {
        g_log.debug() << "Adding WorkspaceIndex " << i << " to mask.\n";
        tmp_index.push_back(i);
      }
    }
  }
  tmp_index.swap(indexList);
} // appendToIndexListFromWS

} // namespace DataHandling
} // namespace Mantid
