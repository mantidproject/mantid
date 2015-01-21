#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace DataObjects {
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
}

//--------------------------------------------------------------------------

/**
 * Constructor - Default.
 * @return MaskWorkspace
 */
MaskWorkspace::MaskWorkspace() {}

/**
 * Constructor - with a given dimension.
 * @param[in] numvectors Number of vectors/histograms for this workspace.
 * @return MaskWorkspace
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
 * @return MaskWorkspace
 */
MaskWorkspace::MaskWorkspace(Mantid::Geometry::Instrument_const_sptr instrument,
                             const bool includeMonitors)
    : SpecialWorkspace2D(instrument, includeMonitors) {
  this->clearMask();
}

/**
 * Constructor - using a MatrixWorkspace.
 * @param[in] parent:  A matrix workspace that is the base for this workspace.
 * It must have an instrument.
 * @return MaskWorkspace
 */
MaskWorkspace::MaskWorkspace(const API::MatrixWorkspace_const_sptr parent)
    : SpecialWorkspace2D(parent) {
  this->clearMask();
}

//--------------------------------------------------------------------------

/**
 * Destructor
 */
MaskWorkspace::~MaskWorkspace() {}

//--------------------------------------------------------------------------

void MaskWorkspace::clearMask() {
  std::size_t nHist = this->getNumberHistograms();
  for (std::size_t i = 0; i < nHist; ++i) {
    this->dataY(i)[0] = LIVE_VALUE;
    this->dataE(i)[0] = ERROR_VALUE;
  }

  // Clear the mask flags
  Geometry::ParameterMap &pmap = this->instrumentParameters();
  pmap.clearParametersByName("masked");
}

/**
 * @return The total number of masked spectra.
 */
size_t MaskWorkspace::getNumberMasked() const {
  // Determine whether has instrument or not
  Geometry::Instrument_const_sptr inst = getInstrument();

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
      errss << "No instrument is associated with mask workspace "
            << this->name();
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

  Geometry::Instrument_const_sptr inst = this->getInstrument();

  /*
   * Note
   * This test originally just checked if there was an instument and ignored
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
      msg << "There is no instrument associated with workspace \'"
          << this->getName() << "\'";
    else
      msg << "There is no proper instrument associated with workspace \'"
          << this->getName() << "\'.  Number of detectors = "
          << this->getInstrument()->getNumberDetectors();
    throw std::runtime_error(msg.str());
  }

  // return true if the value isn't zero
  if (this->getValue(detectorID, LIVE_VALUE) != LIVE_VALUE) {
    return true;
  }

  // the mask bit on the workspace can be set
  return this->getInstrument()->isDetectorMasked(detectorID);
}

/**
 * @return True if the data should be deleted.
 */
bool MaskWorkspace::isMasked(const std::set<detid_t> &detectorIDs) const {
  if (detectorIDs.empty()) {
    return false;
  }

  bool masked(true);
  for (std::set<detid_t>::const_iterator it = detectorIDs.begin();
       it != detectorIDs.end(); ++it) {
    if (!this->isMasked(*it)) {
      masked = false;
      break; // allows space for a debug print statement
    }
  }
  return masked;
}

/**
 * Use this method with MaskWorkspace that doesn't have an instrument.
 */
bool MaskWorkspace::isMaskedIndex(const std::size_t wkspIndex) const {
  return (this->dataY(wkspIndex)[0] !=
          LIVE_VALUE); // if is not live it should masked
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
void MaskWorkspace::setMasked(const std::set<detid_t> &detectorIDs,
                              const bool mask) {
  for (auto detId = detectorIDs.begin(); detId != detectorIDs.end(); ++detId) {
    this->setMasked(*detId, mask);
  }
}

void MaskWorkspace::setMaskedIndex(const std::size_t wkspIndex,
                                   const bool mask) {
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
void
MaskWorkspace::copyFrom(boost::shared_ptr<const SpecialWorkspace2D> sourcews) {
  SpecialWorkspace2D::copyFrom(sourcews);
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
    if (inst->getNumberDetectors() > 0)
      hasinst = true;
    else
      hasinst = false;
  } else
    hasinst = false;

  return hasinst;
}

} // namespace DataObjects
} // namespace Mantid

///\cond TEMPLATE

namespace Mantid {
namespace Kernel {

template <>
DLLExport Mantid::DataObjects::MaskWorkspace_sptr
IPropertyManager::getValue<Mantid::DataObjects::MaskWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr> *prop =
      dynamic_cast<
          PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected MaskWorkspace.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::MaskWorkspace_const_sptr
IPropertyManager::getValue<Mantid::DataObjects::MaskWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr> *prop =
      dynamic_cast<
          PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected const MaskWorkspace.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
