#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ISpectrum::ISpectrum() : m_specNo(0), detectorIDs(), refDx(), m_hasDx(false) {}

/** Constructor with spectrum number
 * @param specNo :: spectrum # of the spectrum
 */
ISpectrum::ISpectrum(const specnum_t specNo)
    : m_specNo(specNo), detectorIDs(), refDx(), m_hasDx(false) {}

//----------------------------------------------------------------------------------------------
/** Copy spectrum number and detector IDs, but not X vector, from another
 *ISpectrum
 *
 * @param other :: take the values from other, put them in this
 */
void ISpectrum::copyInfoFrom(const ISpectrum &other) {
  m_specNo = other.m_specNo;
  detectorIDs = other.detectorIDs;
}

/**
 * Return the min/max X values for this spectrum.
 * @returns A pair where the first is the minimum X value
 *          and the second the maximum
*/
std::pair<double, double> ISpectrum::getXDataRange() const {
  const auto &xdata = readX();
  return std::pair<double, double>(xdata.front(), xdata.back());
}

/// Sets the x error data.
/// @param Dx :: vector of X error data
void ISpectrum::setDx(const MantidVecPtr &Dx) {
  refDx = Dx;
  m_hasDx = true;
}

/** Returns the x error data
 *  BE VERY CAUTIOUS about using this method (when, e.g., just copying
 *  data from an input to output workspace) if you are not actively
 *  using X errors. It may result in the breaking of sharing between
 *  Dx vectors and a significant and unnecessary bloating of memory usage.
 */
MantidVec &ISpectrum::dataDx() {
  m_hasDx = true;
  return refDx.access();
}

/// Returns the x error data const
const MantidVec &ISpectrum::dataDx() const { return *refDx; }

/// Returns the x error data const
const MantidVec &ISpectrum::readDx() const { return *refDx; }

/// Returns the y data const
const MantidVec &ISpectrum::readY() const { return this->dataY(); }

/// Returns the y error data const
const MantidVec &ISpectrum::readE() const { return this->dataE(); }

/// Returns a pointer to the x data
MantidVecPtr ISpectrum::ptrDx() const {
  m_hasDx = true;
  return refDx;
}

// =============================================================================================
// --------------------------------------------------------------------------
/** Add a detector ID to the set of detector IDs
 *
 * @param detID :: detector ID to insert in set.
 */
void ISpectrum::addDetectorID(const detid_t detID) {
  this->detectorIDs.insert(detID);
}

/** Add a set of detector IDs to the set of detector IDs
 *
 * @param detIDs :: set of detector IDs to insert in set.
 */
void ISpectrum::addDetectorIDs(const std::set<detid_t> &detIDs) {
  if (detIDs.empty())
    return;
  this->detectorIDs.insert(detIDs.begin(), detIDs.end());
}

/** Add a vector of detector IDs to the set of detector IDs
 *
 * @param detIDs :: vector of detector IDs to insert in set.
 */
void ISpectrum::addDetectorIDs(const std::vector<detid_t> &detIDs) {
  if (detIDs.empty())
    return;
  this->detectorIDs.insert(detIDs.begin(), detIDs.end());
}

// --------------------------------------------------------------------------
/** Clear the list of detector IDs, then add one.
 *
 * @param detID :: detector ID to insert in set.
 */
void ISpectrum::setDetectorID(const detid_t detID) {
  this->detectorIDs.clear();
  this->detectorIDs.insert(detID);
}

/** Set the detector IDs to be the set given.
 *  Will clear any previous IDs (unlike addDetectorIDs).
 *  @param detIDs The new list of detector ID numbers
 */
void ISpectrum::setDetectorIDs(const std::set<detid_t> &detIDs) {
  detectorIDs = detIDs;
}

/** Set the detector IDs to be the set given (move version).
 *  Will clear any previous IDs (unlike addDetectorIDs).
 *  @param detIDs The new list of detector ID numbers
 */
void ISpectrum::setDetectorIDs(std::set<detid_t> &&detIDs) {
#if !(defined(__INTEL_COMPILER))
  detectorIDs = std::move(detIDs);
#else
  detectorIDs = detIDs; // No moving on the Mac :(
#endif
}

// --------------------------------------------------------------------------
/** Return true if the given detector ID is in the list for this ISpectrum */
bool ISpectrum::hasDetectorID(const detid_t detID) const {
  return (detectorIDs.find(detID) != detectorIDs.end());
}

// --------------------------------------------------------------------------
/** Get a const reference to the detector IDs set.
 */
const std::set<detid_t> &ISpectrum::getDetectorIDs() const {
  return this->detectorIDs;
}

// --------------------------------------------------------------------------
/** Clear the detector IDs set.
 */
void ISpectrum::clearDetectorIDs() {
  this->detectorIDs.clear();
  return;
}

// --------------------------------------------------------------------------
/** Get a mutable reference to the detector IDs set.
 */
std::set<detid_t> &ISpectrum::getDetectorIDs() { return this->detectorIDs; }

// ---------------------------------------------------------
/// @return the spectrum number of this spectrum
specnum_t ISpectrum::getSpectrumNo() const { return m_specNo; }

/** Sets the the spectrum number of this spectrum
 * @param num :: the spectrum number of this spectrum */
void ISpectrum::setSpectrumNo(specnum_t num) { m_specNo = num; }

// ---------------------------------------------------------
/** Lock access to the data so that it does not get deleted while reading.
 * Does nothing unless overridden.
 */
void ISpectrum::lockData() const {}

/** Unlock access to the data so that it can again get deleted.
 * Does nothing unless overridden.
 */
void ISpectrum::unlockData() const {}

//---------------------------------------------------------
/**
 * Gets the value of the use flag.
 * @returns true if DX has been set, else false
 */
bool ISpectrum::hasDx() const { return m_hasDx; }

/**
 * Resets the hasDx flag
 */
void ISpectrum::resetHasDx() { m_hasDx = false; }

} // namespace Mantid
} // namespace API
