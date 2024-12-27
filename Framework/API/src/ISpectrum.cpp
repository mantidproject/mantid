// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidHistogramData/Histogram.h"

namespace Mantid::API {

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
  invalidateCachedSpectrumNumbers();
  invalidateSpectrumDefinition();
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
  size_t oldSize = detectorIDs.size();
  this->detectorIDs.insert(detID);
  if (detectorIDs.size() != oldSize)
    invalidateSpectrumDefinition();
}

/** Add a set of detector IDs to the set of detector IDs
 *
 * @param detIDs :: set of detector IDs to insert in set.
 */
void ISpectrum::addDetectorIDs(const std::set<detid_t> &detIDs) {
  size_t oldSize = detectorIDs.size();
  this->detectorIDs.insert(detIDs.begin(), detIDs.end());
  if (detectorIDs.size() != oldSize)
    invalidateSpectrumDefinition();
}

/** Add a vector of detector IDs to the set of detector IDs
 *
 * @param detIDs :: vector of detector IDs to insert in set.
 */
void ISpectrum::addDetectorIDs(const std::vector<detid_t> &detIDs) {
  size_t oldSize = detectorIDs.size();
  this->detectorIDs.insert(detIDs.begin(), detIDs.end());
  if (detectorIDs.size() != oldSize)
    invalidateSpectrumDefinition();
}

/** Clear the list of detector IDs, then add one.
 *
 * @param detID :: detector ID to insert in set.
 */
void ISpectrum::setDetectorID(const detid_t detID) {
  this->detectorIDs.clear();
  this->detectorIDs.insert(detID);
  invalidateSpectrumDefinition();
}

/** Set the detector IDs to be the set given.
 *  Will clear any previous IDs (unlike addDetectorIDs).
 *  @param detIDs The new list of detector ID numbers
 */
void ISpectrum::setDetectorIDs(const std::set<detid_t> &detIDs) {
  detectorIDs = detIDs;
  invalidateSpectrumDefinition();
}

/** Set the detector IDs to be the set given (move version).
 *  Will clear any previous IDs (unlike addDetectorIDs).
 *  @param detIDs The new list of detector ID numbers
 */
void ISpectrum::setDetectorIDs(std::set<detid_t> &&detIDs) {
  detectorIDs = std::move(detIDs);
  invalidateSpectrumDefinition();
}

/** Return true if the given detector ID is in the list for this ISpectrum */
bool ISpectrum::hasDetectorID(const detid_t detID) const { return (detectorIDs.find(detID) != detectorIDs.end()); }

/** Get a const reference to the detector IDs set.
 */
const std::set<detid_t> &ISpectrum::getDetectorIDs() const { return this->detectorIDs; }

/** Clear the detector IDs set.
 */
void ISpectrum::clearDetectorIDs() {
  this->detectorIDs.clear();
  invalidateSpectrumDefinition();
}

/// @return the spectrum number of this spectrum
specnum_t ISpectrum::getSpectrumNo() const { return m_specNo; }

/** Sets the spectrum number of this spectrum
 * @param num :: the spectrum number of this spectrum */
void ISpectrum::setSpectrumNo(specnum_t num) {
  m_specNo = num;
  invalidateCachedSpectrumNumbers();
}

/**
 * Gets the value of the use flag.
 * @returns true if DX has been set, else false
 */
bool ISpectrum::hasDx() const { return bool(histogramRef().sharedDx()); }

/**
 * Resets the hasDx flag
 */
void ISpectrum::resetHasDx() { mutableHistogramRef().setSharedDx(nullptr); }

/// Copy constructor.
ISpectrum::ISpectrum(const ISpectrum &other) : m_specNo(other.m_specNo), detectorIDs(other.detectorIDs) {
  // m_matrixWorkspace and m_index are not copied: A copy should not refer to
  // the parent of the source. m_experimentInfo will be nullptr.
}

/// Move constructor.
ISpectrum::ISpectrum(ISpectrum &&other) : m_specNo(other.m_specNo), detectorIDs(std::move(other.detectorIDs)) {
  // m_matrixWorkspace and m_index are not copied: A copy should not refer to
  // the parent of the source. m_experimentInfo will be nullptr.
}

/// Copy assignment.
ISpectrum &ISpectrum::operator=(const ISpectrum &other) {
  m_specNo = other.m_specNo;
  detectorIDs = other.detectorIDs;
  // m_matrixWorkspace and m_index are not assigned: The lhs of the assignment
  // keeps its current values.
  invalidateCachedSpectrumNumbers();
  invalidateSpectrumDefinition();
  return *this;
}

/// Move assignment.
ISpectrum &ISpectrum::operator=(ISpectrum &&other) {
  m_specNo = other.m_specNo;
  detectorIDs = std::move(other.detectorIDs);
  // m_matrixWorkspace and m_index are not assigned: The lhs of the assignment
  // keeps its current values.
  invalidateCachedSpectrumNumbers();
  invalidateSpectrumDefinition();
  return *this;
}

/** Sets the MatrixWorkspace pointer (pointer to the owning workspace).
 *
 * This method should not need to be called explicitly, it is called when
 * getting a mutable reference to an ISpectrum stored in a MatrixWorkspace. The
 * pointer set by this method is used to push updates of the detector IDs into
 * the Beamline::SpectrumInfo that is stored in the ExperimentInfo. */
void ISpectrum::setMatrixWorkspace(MatrixWorkspace *matrixWorkspace, const size_t index) {
  m_matrixWorkspace = matrixWorkspace;
  m_index = index;
}

/// Invalidates cached spectrum numbers in the owning workspace.
void ISpectrum::invalidateCachedSpectrumNumbers() const {
  if (m_matrixWorkspace)
    m_matrixWorkspace->invalidateCachedSpectrumNumbers();
}

/// Invalidates spectrum definitions in the owning workspace.
void ISpectrum::invalidateSpectrumDefinition() const {
  if (m_matrixWorkspace)
    m_matrixWorkspace->invalidateSpectrumDefinition(m_index);
}

/// Override in child classes for polymorphic copying of data.
void ISpectrum::copyDataInto(DataObjects::EventList & /*unused*/) const {
  throw std::runtime_error("Incompatible types in ISpectrum::copyDataFrom");
}
/// Override in child classes for polymorphic copying of data.
void ISpectrum::copyDataInto(DataObjects::Histogram1D & /*unused*/) const {
  throw std::runtime_error("Incompatible types in ISpectrum::copyDataFrom");
}

/// Override in child classes for polymorphic copying of data.
void ISpectrum::copyDataInto(SpectrumTester & /*unused*/) const {
  throw std::runtime_error("Incompatible types in ISpectrum::copyDataFrom");
}

} // namespace Mantid::API
