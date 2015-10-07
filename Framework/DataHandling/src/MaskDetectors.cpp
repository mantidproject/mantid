//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MaskDetectors.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include <set>

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

/// (Empty) Constructor
MaskDetectors::MaskDetectors() {}

/// Destructor
MaskDetectors::~MaskDetectors() {}

/*
 * Define input arguments
 */
void MaskDetectors::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("Workspace", "", Direction::InOut),
      "The name of the input and output workspace on which to perform the "
      "algorithm.");
  declareProperty(new ArrayProperty<specid_t>("SpectraList"),
                  "An ArrayProperty containing a list of spectra to mask");
  declareProperty(
      new ArrayProperty<detid_t>("DetectorList"),
      "An ArrayProperty containing a list of detector ID's to mask");
  declareProperty(new ArrayProperty<size_t>("WorkspaceIndexList"),
                  "An ArrayProperty containing the workspace indices to mask");
  declareProperty(new WorkspaceProperty<>("MaskedWorkspace", "",
                                          Direction::Input,
                                          PropertyMode::Optional),
                  "If given but not as a SpecialWorkspace2D, the masking from "
                  "this workspace will be copied. If given as a "
                  "SpecialWorkspace2D, the masking is read from its Y values.");

  auto mustBePosInt = boost::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  declareProperty("StartWorkspaceIndex", 0, mustBePosInt,
                  "The index of the first workspace index of input "
                  "MaskedWorkspace to be included in the calculation. Default "
                  "is 0.");
  declareProperty("EndWorkspaceIndex", EMPTY_INT(), mustBePosInt,
                  "The index number of the last workspace index of input "
                  "MaskedWorkspace to be included in the calculation. Default "
                  "is the last histogram.");
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
  std::vector<specid_t> spectraList = getProperty("SpectraList");
  const std::vector<detid_t> detectorList = getProperty("DetectorList");
  const MatrixWorkspace_sptr prevMasking = getProperty("MaskedWorkspace");

  // each one of these values is optional but the user can't leave all four
  // blank
  if (indexList.empty() && spectraList.empty() && detectorList.empty() &&
      !prevMasking) {
    g_log.information(
        name() +
        ": There is nothing to mask, the index, spectra, "
        "detector lists and masked workspace properties are all empty");
    return;
  }

  // If the spectraList property has been set, need to loop over the workspace
  // looking for the
  // appropriate spectra number and adding the indices they are linked to the
  // list to be processed
  if (!spectraList.empty()) {
    fillIndexListFromSpectra(indexList, spectraList, WS);
  } // End dealing with spectraList
  else if (!detectorList.empty()) {
    // Convert from detectors to workspace indexes
    WS->getIndicesFromDetectorIDs(detectorList, indexList);
  }
  // If we have a workspace that could contain masking,copy that in too

  if (prevMasking) {
    DataObjects::MaskWorkspace_const_sptr maskWS =
        boost::dynamic_pointer_cast<DataObjects::MaskWorkspace>(prevMasking);
    if (maskWS) {
      if (maskWS->getInstrument()->getDetectorIDs().size() !=
          WS->getInstrument()->getDetectorIDs().size()) {
        throw std::runtime_error(
            "Size mismatch between input Workspace and MaskWorkspace");
      }

      g_log.debug() << "Extracting mask from MaskWorkspace (" << maskWS->name()
                    << ")" << std::endl;
      appendToIndexListFromMaskWS(indexList, maskWS);
    } else {
      // Check the provided workspace has the same number of spectra as the
      // input
      if (prevMasking->getNumberHistograms() > WS->getNumberHistograms()) {
        g_log.error() << "Input workspace has " << WS->getNumberHistograms()
                      << " histograms   vs. "
                      << "Input masking workspace has "
                      << prevMasking->getNumberHistograms() << " histograms. "
                      << std::endl;
        throw std::runtime_error("Size mismatch between two input workspaces.");
      }
      appendToIndexListFromWS(indexList, prevMasking);
    }
  }

  // Need to get hold of the parameter map
  Geometry::ParameterMap &pmap = WS->instrumentParameters();

  // If explicitly given a list of detectors to mask, just mark those.
  // Otherwise, mask all detectors pointing to the requested spectra in
  // indexlist loop below
  std::vector<detid_t>::const_iterator it;
  Instrument_const_sptr instrument = WS->getInstrument();
  if (!detectorList.empty()) {
    for (it = detectorList.begin(); it != detectorList.end(); ++it) {
      try {
        if (const Geometry::ComponentID det =
                instrument->getDetector(*it)->getComponentID()) {
          pmap.addBool(det, "masked", true);
        }
      } catch (Kernel::Exception::NotFoundError &e) {
        g_log.warning() << e.what() << " Found while running MaskDetectors"
                        << std::endl;
      }
    }
  }

  if (indexList.empty()) {
    g_log.warning("No spectra affected.");
    return;
  }

  // Get a reference to the spectra-detector map to get hold of detector ID's
  double prog = 0.0;
  std::vector<size_t>::const_iterator wit;
  for (wit = indexList.begin(); wit != indexList.end(); ++wit) {
    WS->maskWorkspaceIndex(*wit);

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

  /*
  This rebuild request call, gives the workspace the opportunity to rebuild the
  nearest neighbours map
  and therfore pick up any detectors newly masked with this algorithm.
  */
  WS->rebuildNearestNeighbours();
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

  // Need to get hold of the parameter map and instrument
  Geometry::ParameterMap &pmap = WS->instrumentParameters();
  Instrument_const_sptr instrument = WS->getInstrument();

  // If we have a workspace that could contain masking,copy that in too

  if (prevMasking) {
    DataObjects::MaskWorkspace_sptr maskWS =
        boost::dynamic_pointer_cast<DataObjects::MaskWorkspace>(prevMasking);
    if (maskWS) {
      Geometry::ParameterMap &maskPmap = maskWS->instrumentParameters();
      Instrument_const_sptr maskInstrument = maskWS->getInstrument();
      if (maskInstrument->getDetectorIDs().size() !=
          WS->getInstrument()->getDetectorIDs().size()) {
        throw std::runtime_error(
            "Size mismatch between input Workspace and MaskWorkspace");
      }

      g_log.debug() << "Extracting mask from MaskWorkspace (" << maskWS->name()
                    << ")" << std::endl;
      std::vector<detid_t> detectorIDs = maskInstrument->getDetectorIDs();
      std::vector<detid_t>::const_iterator it;
      for (it = detectorIDs.begin(); it != detectorIDs.end(); ++it) {
        try {
          if (const Geometry::ComponentID det =
                  maskInstrument->getDetector(*it)->getComponentID()) {
            Geometry::Parameter_sptr maskedParam = maskPmap.get(det, "masked");
            int detID =
                static_cast<int>(maskInstrument->getDetector(*it)->getID());
            if (maskedParam)
              detectorList.push_back(detID);
          }
        } catch (Kernel::Exception::NotFoundError &e) {
          g_log.warning() << e.what() << " Found while running MaskDetectors"
                          << std::endl;
        }
      }
    }
  }

  // If explicitly given a list of detectors to mask, just mark those.
  // Otherwise, mask all detectors pointing to the requested spectra in
  // indexlist loop below
  std::vector<detid_t>::const_iterator it;
  if (!detectorList.empty()) {
    for (it = detectorList.begin(); it != detectorList.end(); ++it) {
      try {
        if (const Geometry::ComponentID det =
                instrument->getDetector(*it)->getComponentID()) {
          pmap.addBool(det, "masked", true);
        }
      } catch (Kernel::Exception::NotFoundError &e) {
        g_log.warning() << e.what() << " Found while running MaskDetectors"
                        << std::endl;
      }
    }
  }
}
/**
 * Convert a list of spectra numbers into the corresponding workspace indices
 * @param indexList :: An output index list from the given spectra list
 * @param spectraList :: A list of spectra numbers
 * @param WS :: The input workspace to be masked
 */
void MaskDetectors::fillIndexListFromSpectra(
    std::vector<size_t> &indexList, const std::vector<specid_t> &spectraList,
    const API::MatrixWorkspace_sptr WS) {
  // Convert the vector of properties into a set for easy searching
  std::set<int64_t> spectraSet(spectraList.begin(), spectraList.end());
  // Next line means that anything in Clear the index list first
  indexList.clear();
  indexList.reserve(WS->getNumberHistograms());

  for (int i = 0; i < static_cast<int>(WS->getNumberHistograms()); ++i) {
    const specid_t currentSpec = WS->getSpectrum(i)->getSpectrumNo();
    if (spectraSet.find(currentSpec) != spectraSet.end()) {
      indexList.push_back(i);
    }
  }
}

/**
 * Append the indices of the masked spectra from the given workspace list to the
 * given list
 * @param indexList :: An existing list of indices
 * @param maskedWorkspace :: An workspace with masked spectra
 */
void MaskDetectors::appendToIndexListFromWS(
    std::vector<size_t> &indexList,
    const MatrixWorkspace_sptr maskedWorkspace) {
  // Convert the vector of properties into a set for easy searching
  std::set<int64_t> existingIndices(indexList.begin(), indexList.end());
  int endIndex = getProperty("EndWorkspaceIndex");
  if (endIndex == EMPTY_INT())
    endIndex = static_cast<int>(maskedWorkspace->getNumberHistograms() - 1);
  int startIndex = getProperty("StartWorkspaceIndex");

  for (int64_t i = startIndex; i <= endIndex; ++i) {
    IDetector_const_sptr det;
    try {
      det = maskedWorkspace->getDetector(i - startIndex);
    } catch (Exception::NotFoundError &) {
      continue;
    }

    if (det->isMasked() && existingIndices.count(i) == 0) {
      indexList.push_back(i);
    }
  }

  return;
} // appendToIndexListFromWS

/**
 * Append the indices of the masked spectra from the given workspace list to the
 * given list
 * @param indexList :: An existing list of indices
 * @param maskedWorkspace :: An workspace with masked spectra
 */
void MaskDetectors::appendToIndexListFromMaskWS(
    std::vector<size_t> &indexList,
    const DataObjects::MaskWorkspace_const_sptr maskedWorkspace) {
  // Convert the vector of properties into a set for easy searching
  std::set<int64_t> existingIndices(indexList.begin(), indexList.end());
  int endIndex = getProperty("EndWorkspaceIndex");
  if (endIndex == EMPTY_INT())
    endIndex = static_cast<int>(maskedWorkspace->getNumberHistograms() - 1);
  int startIndex = getProperty("StartWorkspaceIndex");

  for (int64_t i = startIndex; i <= endIndex; ++i) {

    if (maskedWorkspace->dataY(i - startIndex)[0] > 0.5 &&
        existingIndices.count(i) == 0) {
      g_log.debug() << "Adding WorkspaceIndex " << i << " to mask."
                    << std::endl;
      indexList.push_back(i);
    }
  }

  return;
} // appendToIndexListFromWS

} // namespace DataHandling
} // namespace Mantid
