// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <algorithm>
#include <utility>

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid::DataObjects {
using Mantid::API::SpectrumInfo;
using Mantid::Geometry::DetectorInfo;
using Mantid::Kernel::Exception::NotImplementedError;
using std::set;
using std::size_t;

// Register the workspace
DECLARE_WORKSPACE(MaskWorkspace)

namespace { // keep these constants only within this file.
/// The only value allowed for a pixel to be kept.
const double LIVE_VALUE = 0.;

/**
 * The default value for marking a pixel to be masked. For checks
 * anything that isn't live is dead.
 */
const double DEAD_VALUE = 1.;

/// The value for uncertainty.
const double ERROR_VALUE = 0.;
} // namespace

//--------------------------------------------------------------------------

/**
 * Constructor - with a given dimension.
 * @param[in] numvectors Number of vectors/histograms for this workspace.
 */
MaskWorkspace::MaskWorkspace(std::size_t numvectors) {
  this->init(numvectors, 1, 1);
  this->clearMask();
}

/**
 * Constructor - using an instrument.
 * @param[in] instrument : Instrument that is the base for this workspace.
 * @param[in] includeMonitors: option for including the monitors at the
 * workspace.
 */
MaskWorkspace::MaskWorkspace(const Mantid::Geometry::Instrument_const_sptr &instrument, const bool includeMonitors)
    : SpecialWorkspace2D(instrument, includeMonitors) {
  this->clearMask();
}

/**
 * Constructor - using a MatrixWorkspace.
 * @param[in] parent:  A matrix workspace that is the base for this workspace.
 * It must have an instrument.
 */
MaskWorkspace::MaskWorkspace(const API::MatrixWorkspace_const_sptr &parent) : SpecialWorkspace2D(parent) {
  this->clearMask();
}

//--------------------------------------------------------------------------

void MaskWorkspace::clearMask() {
  std::size_t nHist = this->getNumberHistograms();
  for (std::size_t i = 0; i < nHist; ++i) {
    this->dataY(i)[0] = LIVE_VALUE;
    this->dataE(i)[0] = ERROR_VALUE;
  }

  // Clear the mask flags
  mutableDetectorInfo().clearMaskFlags();
}

/**
 * @return The total number of masked spectra.
 */
size_t MaskWorkspace::getNumberMasked() const {
  size_t numMasked(0);
  const size_t numWksp(this->getNumberHistograms());
  for (size_t i = 0; i < numWksp; i++) {
    if (this->isMaskedIndex(i)) // quick check the value
    {
      numMasked++;
    } else if (this->hasInstrument()) {
      const set<detid_t> ids = this->getDetectorIDs(i);
      if (this->isMasked(ids)) // slow and correct check with the real method
        numMasked += ids.size();
    } else {
      std::stringstream errss;
      errss << "No instrument is associated with mask workspace " << this->getName();
      throw std::runtime_error(errss.str());
    }
  }
  return numMasked;
}

/**
 * @brief MaskWorkspace::getMaskedDetectors
 * @return Which detector ids are masked.
 */
set<detid_t> MaskWorkspace::getMaskedDetectors() const {
  set<detid_t> detIDs;
  /*
   * Note
   * This test originally just checked if there was an instrument and ignored
   * the number of detectors
   */
  if (this->hasInstrument()) {
    size_t numHist(this->getNumberHistograms());
    for (size_t i = 0; i < numHist; i++) {
      if (this->isMaskedIndex(i)) {
        set<detid_t> temp = this->getDetectorIDs(i);
        detIDs.insert(temp.begin(), temp.end());
      }
    }
  }

  return detIDs;
}

/**
 * @brief MaskWorkspace::getMaskedWkspIndices
 * @return Which workspace indices are masked.
 */
set<size_t> MaskWorkspace::getMaskedWkspIndices() const {
  set<size_t> indices;

  size_t numHist(this->getNumberHistograms());
  for (size_t i = 0; i < numHist; i++) {
    if (this->isMaskedIndex(i)) {
      indices.insert(i);
    }
  }

  return indices;
}

//--------------------------------------------------------------------------------------------
/**
 * @param detectorID :: ID of the detector to check whether it is masked or not
 * @return True if the data should be deleted.
 */
bool MaskWorkspace::isMasked(const detid_t detectorID) const {
  if (!this->hasInstrument()) {
    std::stringstream msg;
    if (!this->getInstrument())
      msg << "There is no instrument associated with workspace \'" << this->getName() << "\'";
    else
      msg << "There is no proper instrument associated with workspace \'" << this->getName()
          << "\'.  Number of detectors = " << this->getInstrument()->getNumberDetectors();
    throw std::runtime_error(msg.str());
  }

  // return true if the value isn't zero
  if (this->getValue(detectorID, LIVE_VALUE) != LIVE_VALUE) {
    return true;
  }

  // the mask bit on the workspace can be set
  // Performance wise, it is not optimal to call detectorInfo() for every index,
  // but this method seems to be used rarely enough to justify this until the
  // Instrument-2.0 implementation has progressed far enough to make this cheap.
  const auto &detectorInfo = this->detectorInfo();
  try {
    return detectorInfo.isMasked(detectorInfo.indexOf(detectorID));
  } catch (std::out_of_range &) {
    // The workspace can contain bad detector IDs. DetectorInfo::indexOf throws.
    return false;
  }
}

/**
 * @return True if the data should be deleted.
 */
bool MaskWorkspace::isMasked(const std::set<detid_t> &detectorIDs) const {
  if (detectorIDs.empty()) {
    return false;
  }

  const auto it = std::find_if_not(detectorIDs.cbegin(), detectorIDs.cend(),
                                   [this](const auto detectorID) { return this->isMasked(detectorID); });
  return it == detectorIDs.cend();
}

/**
 * Use this method with MaskWorkspace that doesn't have an instrument.
 */
bool MaskWorkspace::isMaskedIndex(const std::size_t wkspIndex) const {
  return (this->dataY(wkspIndex)[0] != LIVE_VALUE); // if is not live it should masked
}

/**
 * Mask an individual pixel.
 *
 * @param detectorID to mask.
 * @param mask True means to delete the data.
 */
void MaskWorkspace::setMasked(const detid_t detectorID, const bool mask) {
  double value(LIVE_VALUE);
  if (mask)
    value = DEAD_VALUE;

  this->setValue(detectorID, value, ERROR_VALUE);
}

/**
 * Mask a set of pixels. This is a convenience function to
 * call MaskWorkspace::setMasked(const detid_t, const bool).
 */
void MaskWorkspace::setMasked(const std::set<detid_t> &detectorIDs, const bool mask) {
  for (auto detectorID : detectorIDs) {
    this->setMasked(detectorID, mask);
  }
}

void MaskWorkspace::setMaskedIndex(const std::size_t wkspIndex, const bool mask) {
  double value(LIVE_VALUE);
  if (mask)
    value = DEAD_VALUE;

  this->dataY(wkspIndex)[0] = value;
}

/**
 * Gets the name of the workspace type.
 * @return Standard string name
 */
const std::string MaskWorkspace::id() const { return "MaskWorkspace"; }

//--------------------------------------------------------------------------------------------
/** Copy from
 */
void MaskWorkspace::copyFrom(std::shared_ptr<const SpecialWorkspace2D> sourcews) {
  SpecialWorkspace2D::copyFrom(sourcews);
}

/// Ensure that the detector mask flags include the values from this workspace.
void MaskWorkspace::combineToDetectorMasks(DetectorInfo &detectors) const {
  if (!hasInstrument())
    throw std::logic_error("MaskWorkspace has no instrument");

  // detectors which are monitors are not included in the mask
  const auto &detids = getInstrument()->getDetectorIDs(true);
  const size_t N_spectra(getNumberHistograms());
  const size_t N_monitors(detectorInfo().size() - detids.size());
  const size_t N_detectors(detectors.size());
  if (N_spectra == N_detectors - N_monitors) {
    // One detector per spectrum
    for (const auto &det : detids) {
      if (isMasked(det))
        detectors.setMasked(detectors.indexOf(det), true);
    }
    return;
  }
  // Multiple detectors per spectrum
  throw NotImplementedError(
      "MaskWorkspace::combineToDetectorMasks: not implemented for the case of multiple detectors per spectrum.");
#if 0 // Probably works: requires unit tests implementation.
  const SpectrumInfo &info(spectrumInfo());
  for (size_t ns = 0; ns < N_spectra; ++ns) {
    const auto &spectrumDef = info.spectrumDefinition(ns);
    if (isMaskedIndex(ns)) {
      for (const auto &det_idx : spectrumDef)
        detectors.setMasked(det_idx, true);
    }
  }
#endif
}

} // namespace Mantid::DataObjects

#if 0 // Probably works: requires unit tests implementation.
namespace { // anonymous

// Note: the following code replicates SpectrumInfo.cpp: lines 61-66, but as an anonymous utility method:
bool isMaskedSpectrum(const Mantid::SpectrumDefinition &spectrumDef,
                      const Mantid::Geometry::DetectorInfo &detectorInfo) {
  return std::all_of(
      spectrumDef.cbegin(), spectrumDef.cend(),
      [&detectorInfo](const std::pair<size_t, size_t> &detIndex) { return detectorInfo.isMasked(detIndex); });
}

} // namespace
#endif

namespace Mantid::DataObjects {

/// Ensure that this workspace includes the values from the detector mask flags.
void MaskWorkspace::combineFromDetectorMasks(const DetectorInfo &detectors) {
  if (!hasInstrument())
    throw std::logic_error("MaskWorkspace has no instrument");

  // detectors which are monitors are not included in the mask
  const auto &detids = getInstrument()->getDetectorIDs(true);
  const size_t N_spectra(getNumberHistograms());
  const size_t N_monitors(detectorInfo().size() - detids.size());
  const size_t N_detectors(detectors.size());
  if (N_spectra == N_detectors - N_monitors) {
    // One detector per spectrum
    for (const auto &det : detids) {
      if (detectors.isMasked(detectors.indexOf(det)))
        setMasked(det, true);
    }
    return;
  }
  // Multiple detectors per spectrum
  throw NotImplementedError(
      "MaskWorkspace::combineFromDetectorMasks: not implemented for the case of multiple detectors per spectrum.");
#if 0 // Probably works: requires unit tests implementation.
  const SpectrumInfo &info(spectrumInfo());
  for (size_t ns = 0; ns < N_spectra; ++ns) {
    const auto &spectrumDef = info.spectrumDefinition(ns);
    if (isMaskedSpectrum(spectrumDef, detectors)) {
      setMaskedIndex(ns, true);
    }
  }
#endif
}

/// Test consistency between the values from this workspace and the detector mask flags.
bool MaskWorkspace::isConsistentWithDetectorMasks(const DetectorInfo &detectors) const {
  if (!hasInstrument())
    throw std::logic_error("MaskWorkspace has no instrument");

  // detectors which are monitors are not included in the mask
  const auto &detids = getInstrument()->getDetectorIDs(true);
  const size_t N_spectra(getNumberHistograms());
  const size_t N_monitors(detectorInfo().size() - detids.size());
  const size_t N_detectors(detectors.size());
  if (N_spectra == N_detectors - N_monitors) {
    // One detector per spectrum
    for (const auto &det : detids) {
      if (isMasked(det) != detectors.isMasked(detectors.indexOf(det)))
        return false;
    }
    return true;
  }
  // Multiple detectors per spectrum
  throw NotImplementedError(
      "MaskWorkspace::isConsistentWithDetectorMasks: not implemented for the case of multiple detectors per spectrum.");
#if 0 // Probably works: requires unit tests implementation.
  const SpectrumInfo &info(spectrumInfo());
  for (size_t ns = 0; ns < N_spectra; ++ns) {
    const auto &spectrumDef = info.spectrumDefinition(ns);
    if (isMaskedIndex(ns) != isMaskedSpectrum(spectrumDef, detectors))
      return false;
  }
  return true;
#endif
}

/**
 * @return A string containing the workspace description
 */
const std::string MaskWorkspace::toString() const {
  std::ostringstream os;
  os << SpecialWorkspace2D::toString();
  os << "Masked: " << getNumberMasked() << "\n";
  return os.str();
}

//--------------------------------------------------------------------------------------------
/** Check whether workspace has a non-trivial instrument
 * (1) There is an instrument associated with
 * (2) Number of detectors is larger than 0
 */
bool MaskWorkspace::hasInstrument() const {
  bool hasinst;
  Geometry::Instrument_const_sptr inst = this->getInstrument();
  if (inst) {
    hasinst = inst->getNumberDetectors() > 0;
  } else
    hasinst = false;

  return hasinst;
}

} // namespace Mantid::DataObjects

///\cond TEMPLATE

namespace Mantid::Kernel {

template <>
DLLExport Mantid::DataObjects::MaskWorkspace_sptr
IPropertyManager::getValue<Mantid::DataObjects::MaskWorkspace_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<MaskWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::MaskWorkspace_const_sptr
IPropertyManager::getValue<Mantid::DataObjects::MaskWorkspace_const_sptr>(const std::string &name) const {
  const auto *prop =
      dynamic_cast<PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<MaskWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel

///\endcond TEMPLATE
