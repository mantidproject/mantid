//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Axis.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitFactory.h"

#include <iostream>

namespace Mantid
{
namespace API
{

/// Constructor
Axis::Axis() :
m_title(),m_unit(Mantid::Kernel::UnitFactory::Instance().create("Empty"))
{
}

/// Protected copy constructor
Axis::Axis(const Axis& right) :
  m_title(right.m_title), m_unit(right.m_unit)
{
}

/// Virtual destructor
Axis::~Axis()
{}

/// Returns the user-defined title for this axis
const std::string& Axis::title() const
{
  return m_title;
}

/// Returns a reference to the user-defined title for this axis
std::string& Axis::title()
{
  return m_title;
}

/** The unit for this axis
 *  @return A shared pointer to the unit object
 */
const Kernel::Unit_sptr& Axis::unit() const
{
  return m_unit;
}

/** The unit object for this workspace (non const version)
 *  @return A shared pointer to the unit object
 */
Kernel::Unit_sptr& Axis::unit()
{
  return m_unit;
}
/**
* Sets the Unit that is in use on this axis.
* @param unit name of the unit as known to the UnitFactory
*/
void Axis::setUnit(const std::string & unit)
{
  m_unit = Mantid::Kernel::UnitFactory::Instance().create(unit);
}

/** Returns the spectrum number at the position given (Spectra axis only)
 *  @param  index The position for which the value is required
 *  @return The spectrum number as an int
 *  @throw  domain_error If this method is called on a numeric axis
 */
const int& Axis::spectraNo(const int& index) const
{
  (void) index; //Avoid compiler warning
  throw std::domain_error("Cannot call spectraNo() on a non-spectra axis.");
}

/** Returns a non-const reference to the spectrum number at the position given (Spectra axis only)
 *  @param  index The position for which the value is required
 *  @return The spectrum number as an int
 *  @throw  domain_error If this method is called on a numeric axis
 */
int& Axis::spectraNo(const int& index)
{
  (void) index; //Avoid compiler warning
  throw std::domain_error("Cannot call spectraNo() on a non-spectra axis.");
}

} // namespace API
} // namespace Mantid
