#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace API {

/** Constructor with spectrum number
 * @param specNo :: spectrum # of the spectrum
 */
ISpectrum::ISpectrum(const specnum_t specNo) : m_specNo(specNo) {}

/** Copy spectrum number and detector IDs, but not X vector, from another
 *ISpectrum
 *
 * @param other :: take the values from other, put them in this
 */
void ISpectrum::copyInfoFrom(const ISpectrum &other) {
  m_specNo = other.m_specNo;
  detectorIDs = other.detectorIDs;
  updateExperimentInfo();
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

/// Deprecated, use y() instead. Returns the y data const
const MantidVec &ISpectrum::readY() const { return this->dataY(); }

/// Deprecated, use e() instead. Returns the y error data const
const MantidVec &ISpectrum::readE() const { return this->dataE(); }

/** Add a detector ID to the set of detector IDs
 *
 * @param detID :: detector ID to insert in set.
 */
void ISpectrum::addDetectorID(const detid_t detID) {
  this->detectorIDs.insert(detID);
  if (m_experimentInfo)
    m_experimentInfo->addDetectorToGroup(m_index, detID);
}

/** Add a set of detector IDs to the set of detector IDs
 *
 * @param detIDs :: set of detector IDs to insert in set.
 */
void ISpectrum::addDetectorIDs(const std::set<detid_t> &detIDs) {
  this->detectorIDs.insert(detIDs.begin(), detIDs.end());
  if (m_experimentInfo)
    m_experimentInfo->addDetectorsToGroup(m_index, detIDs);
}

/** Add a vector of detector IDs to the set of detector IDs
 *
 * @param detIDs :: vector of detector IDs to insert in set.
 */
void ISpectrum::addDetectorIDs(const std::vector<detid_t> &detIDs) {
  this->detectorIDs.insert(detIDs.begin(), detIDs.end());
  if (m_experimentInfo)
    m_experimentInfo->addDetectorsToGroup(
        m_index, std::set<detid_t>(detIDs.begin(), detIDs.end()));
}

/** Clear the list of detector IDs, then add one.
 *
 * @param detID :: detector ID to insert in set.
 */
void ISpectrum::setDetectorID(const detid_t detID) {
  this->detectorIDs.clear();
  this->detectorIDs.insert(detID);
  updateExperimentInfo();
}

/** Set the detector IDs to be the set given.
 *  Will clear any previous IDs (unlike addDetectorIDs).
 *  @param detIDs The new list of detector ID numbers
 */
void ISpectrum::setDetectorIDs(const std::set<detid_t> &detIDs) {
  detectorIDs = detIDs;
  updateExperimentInfo();
}

/** Set the detector IDs to be the set given (move version).
 *  Will clear any previous IDs (unlike addDetectorIDs).
 *  @param detIDs The new list of detector ID numbers
 */
void ISpectrum::setDetectorIDs(std::set<detid_t> &&detIDs) {
  detectorIDs = std::move(detIDs);
  updateExperimentInfo();
}

/** Return true if the given detector ID is in the list for this ISpectrum */
bool ISpectrum::hasDetectorID(const detid_t detID) const {
  return (detectorIDs.find(detID) != detectorIDs.end());
}

/** Get a const reference to the detector IDs set.
 */
const std::set<detid_t> &ISpectrum::getDetectorIDs() const {
  return this->detectorIDs;
}

/** Clear the detector IDs set.
 */
void ISpectrum::clearDetectorIDs() {
  this->detectorIDs.clear();
  updateExperimentInfo();
}

/// @return the spectrum number of this spectrum
specnum_t ISpectrum::getSpectrumNo() const { return m_specNo; }

/** Sets the the spectrum number of this spectrum
 * @param num :: the spectrum number of this spectrum */
void ISpectrum::setSpectrumNo(specnum_t num) { m_specNo = num; }

/**
 * Gets the value of the use flag.
 * @returns true if DX has been set, else false
 */
bool ISpectrum::hasDx() const { return bool(histogramRef().sharedDx()); }

/**
 * Resets the hasDx flag
 */
void ISpectrum::resetHasDx() { mutableHistogramRef().setSharedDx(nullptr); }

ISpectrum::ISpectrum(const ISpectrum &other)
    : m_specNo(other.m_specNo), detectorIDs(other.detectorIDs) {
  // m_experimentInfo and m_index are not copied: A copy should not refer to the
  // parent of the source. m_experimentInfo will be nullptr.
}

ISpectrum::ISpectrum(ISpectrum &&other)
    : m_specNo(other.m_specNo), detectorIDs(std::move(other.detectorIDs)) {
  // m_experimentInfo and m_index are not copied: A copy should not refer to the
  // parent of the source. m_experimentInfo will be nullptr.
}

ISpectrum &ISpectrum::operator=(const ISpectrum &other) {
  m_specNo = other.m_specNo;
  detectorIDs = other.detectorIDs;
  // m_experimentInfo and m_index are not assigned: The lhs of the assignment
  // keeps its current values.
  updateExperimentInfo();
  return *this;
}

ISpectrum &ISpectrum::operator=(ISpectrum &&other) {
  m_specNo = other.m_specNo;
  detectorIDs = std::move(other.detectorIDs);
  // m_experimentInfo and m_index are not assigned: The lhs of the assignment
  // keeps its current values.
  updateExperimentInfo();
  return *this;
}

void ISpectrum::setExperimentInfo(ExperimentInfo *experimentInfo,
                                  const size_t index) {
  m_experimentInfo = experimentInfo;
  m_index = index;
}

void ISpectrum::updateExperimentInfo() const {
  if (m_experimentInfo)
    m_experimentInfo->updateCachedDetectorGrouping(m_index, detectorIDs);
}

} // namespace Mantid
} // namespace API
