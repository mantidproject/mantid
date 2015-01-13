//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Unit.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

namespace Mantid {
namespace API {

using std::size_t;

/** Virtual constructor
 *  @param parentWorkspace The workspace to which this axis belongs
 */
SpectraAxis::SpectraAxis(const MatrixWorkspace *const parentWorkspace)
    : Axis(), m_parentWS(parentWorkspace), m_edges() {
  this->unit() = boost::make_shared<Kernel::Units::Label>("Spectrum", "");
}

/** Virtual constructor
 *  @param parentWorkspace The workspace to which the cloned axis belongs
 *  @return A pointer to a copy of the SpectraAxis on which the method is called
 */
Axis *SpectraAxis::clone(const MatrixWorkspace *const parentWorkspace) {
  SpectraAxis *newAxis = new SpectraAxis(parentWorkspace);
  // A couple of base class members need copying over manually
  newAxis->title() = this->title();
  newAxis->unit() = this->unit();
  return newAxis;
}

/** Virtual constructor
 *  @param length Not used in this implementation
 *  @param parentWorkspace The workspace to which the cloned axis belongs
 *  @return A pointer to a copy of the SpectraAxis on which the method is called
 */
Axis *SpectraAxis::clone(const std::size_t length,
                         const MatrixWorkspace *const parentWorkspace) {
  UNUSED_ARG(length)
  // In this implementation, there's no difference between the clone methods -
  // call the other one
  return clone(parentWorkspace);
}

std::size_t SpectraAxis::length() const {
  return m_parentWS->getNumberHistograms();
}

/** Get the axis value at the position given
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex Needed for the subclass (RefAxis) method, but ignored
 * (and defaulted) here
 *  @return The value of the axis as a double
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
double SpectraAxis::operator()(const std::size_t &index,
                               const std::size_t &verticalIndex) const {
  UNUSED_ARG(verticalIndex)
  if (index >= length()) {
    throw Kernel::Exception::IndexError(index, length() - 1,
                                        "SpectraAxis: Index out of range.");
  }

  return static_cast<double>(m_parentWS->getSpectrum(index)->getSpectrumNo());
}

/** Sets the axis value at a given position
 *  @param index :: The position along the axis for which to set the value
 *  @param value :: The new value
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
void SpectraAxis::setValue(const std::size_t &index, const double &value) {
  UNUSED_ARG(index)
  UNUSED_ARG(value)
  throw std::domain_error("setValue method cannot be used on a SpectraAxis.");
}

/**
 * Finds the index of the given value on the axis
 * @param value A value on the axis. It is treated as a spectrum number and cast
 * to specid_t on input
 * @return The index closest to given value
 * @throws std::out_of_range if the value is out of range of the axis
 */
size_t SpectraAxis::indexOfValue(const double value) const {
  if (m_edges.empty()) // lazy-instantiation
  {
    m_edges.resize(m_parentWS->getNumberHistograms() + 1);
    const size_t npts = m_edges.size() - 1;
    for (size_t i = 0; i < npts - 1; ++i) {
      m_edges[i + 1] = 0.5 * (this->getValue(i) + this->getValue(i + 1));
    }
    // ends
    m_edges[0] = this->getValue(0) - (m_edges[1] - this->getValue(0));
    m_edges[npts] = this->getValue(npts - 1) +
                    (this->getValue(npts - 1) - m_edges[npts - 1]);
  }
  return NumericAxis::indexOfValue(value, m_edges);
}

/** Returns the spectrum number at the position given (Spectra axis only)
 *  @param  index The position for which the value is required
 *  @return The spectrum number as an int
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
specid_t SpectraAxis::spectraNo(const std::size_t &index) const {
  if (index >= length()) {
    throw Kernel::Exception::IndexError(index, length() - 1,
                                        "SpectraAxis: Index out of range.");
  }

  return m_parentWS->getSpectrum(index)->getSpectrumNo();
}

/** Returns a map where spectra is the key and index is the value
 *  This is used for efficient search of spectra number within a workspace
 *  @param  map Reference to the map
 */
void SpectraAxis::getSpectraIndexMap(spec2index_map &map) const {
  size_t nel = length();

  if (nel == 0)
    throw std::runtime_error("getSpectraIndexMap(),  zero elements");
  map.clear();
  for (size_t i = 0; i < nel; ++i) {
    map.insert(std::make_pair(m_parentWS->getSpectrum(i)->getSpectrumNo(), i));
  }
}

/** Check if two axis defined as spectra or numeric axis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true is self and second axis are equal
 */
bool SpectraAxis::operator==(const Axis &axis2) const {
  if (length() != axis2.length()) {
    return false;
  }
  const SpectraAxis *spec2 = dynamic_cast<const SpectraAxis *>(&axis2);
  if (!spec2) {
    return false;
  }
  for (size_t i = 0; i < length(); ++i) {
    if (spectraNo(i) != axis2.spectraNo(i))
      return false;
  }
  // All good if we get to here
  return true;
}

/** Returns a text label which shows the value at index and identifies the
 *  type of the axis.
 *  @param index :: The index of an axis value
 *  @return label of requested axis index
 */
std::string SpectraAxis::label(const std::size_t &index) const {
  return "sp-" + boost::lexical_cast<std::string>(spectraNo(index));
}

/// returns min value defined on axis
double SpectraAxis::getMin() const {
  return m_parentWS->getSpectrum(0)->getSpectrumNo();
}

/// returns max value defined on axis
double SpectraAxis::getMax() const {
  return m_parentWS->getSpectrum(length() - 1)->getSpectrumNo();
}

} // namespace API
} // namespace Mantid
