//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Axis.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitFactory.h"

#include <iostream>

namespace Mantid {
namespace API {

using std::size_t;

/// Constructor
Axis::Axis()
    : m_title(),
      m_unit(Mantid::Kernel::UnitFactory::Instance().create("Empty")) {}

/// Protected copy constructor
Axis::Axis(const Axis &right) : m_title(right.m_title), m_unit(right.m_unit) {}

/// Virtual destructor
Axis::~Axis() {}

/// Returns the user-defined title for this axis
const std::string &Axis::title() const { return m_title; }

/// Returns a reference to the user-defined title for this axis
std::string &Axis::title() { return m_title; }

/** The unit for this axis
 *  @return A shared pointer to the unit object
 */
const Kernel::Unit_sptr &Axis::unit() const { return m_unit; }

/** The unit object for this workspace (non const version)
 *  @return A shared pointer to the unit object
 */
Kernel::Unit_sptr &Axis::unit() { return m_unit; }
/**
* Sets the Unit that is in use on this axis.
* @param unitName :: name of the unit as known to the UnitFactory
* @returns The new unit instance
*/
const Kernel::Unit_sptr &Axis::setUnit(const std::string &unitName) {
  m_unit = Mantid::Kernel::UnitFactory::Instance().create(unitName);
  return unit();
}

/**
 *  Gets the value at the specified index. Easier to use with pointers than the
 * operator()
 *  @param index :: The index along the axis direction
 *  @param verticalIndex :: The verticalIndex (used in RefAxis)
 *  @returns The double value at the given index
 */
double Axis::getValue(const std::size_t &index,
                      const std::size_t &verticalIndex) const {
  return (*this)(index, verticalIndex);
}

/** Returns the spectrum number at the position given (Spectra axis only)
 *  @param  index The position for which the value is required
 *  @return The spectrum number as an int
 *  @throw  domain_error If this method is called on a numeric axis
 */
specid_t Axis::spectraNo(const std::size_t &index) const {
  UNUSED_ARG(index)
  throw std::domain_error("Cannot call spectraNo() on a non-spectra axis.");
}

} // namespace API
} // namespace Mantid
