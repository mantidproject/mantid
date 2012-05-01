//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"

#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace API
{

/** Constructor
 * @param length size of the numeric axis
 */
NumericAxis::NumericAxis(const std::size_t& length): Axis()
{
  m_values.resize(length);
}

/** Virtual constructor
 *  @param parentWorkspace :: The workspace is not used in this implementation
 *  @returns A pointer to a copy of the NumericAxis on which the method is called
 */
Axis* NumericAxis::clone(const MatrixWorkspace* const parentWorkspace)
{
  UNUSED_ARG(parentWorkspace)
  return new NumericAxis(*this);
}

Axis* NumericAxis::clone(const std::size_t length, const MatrixWorkspace* const parentWorkspace)
{
  UNUSED_ARG(parentWorkspace)
  NumericAxis * newAxis = new NumericAxis(*this);
  newAxis->m_values.clear();
  newAxis->m_values.resize(length);
  return newAxis;
}

/** Get the axis value at the position given
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex Needed for the subclass (RefAxis) method, but ignored (and defaulted) here
 *  @return The value of the axis as a double
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
double NumericAxis::operator()(const std::size_t& index, const std::size_t& verticalIndex) const
{
  UNUSED_ARG(verticalIndex)
  if (index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "NumericAxis: Index out of range.");
  }

  return m_values[index];
}

/** Sets the axis value at a given position
 *  @param index :: The position along the axis for which to set the value
 *  @param value :: The new value
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
void NumericAxis::setValue(const std::size_t& index, const double& value)
{
  if (index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "NumericAxis: Index out of range.");
  }

  m_values[index] = value;
}

/** Check if two axis defined as spectra or numeric axis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true if self and second axis are equal
 */
bool NumericAxis::operator==(const Axis& axis2) const
{
	if (length()!=axis2.length())
  {
		return false;
  }
  const NumericAxis* spec2 = dynamic_cast<const NumericAxis*>(&axis2);
  if (!spec2)
  {
    return false;
  }
	return std::equal(m_values.begin(),m_values.end(),spec2->m_values.begin());
}

/** Returns a text label which shows the value at index and identifies the
 *  type of the axis.
 *  @param index :: The index of an axis value
 *  @return the label of the requested axis
 */
std::string NumericAxis::label(const std::size_t& index)const
{
  return boost::lexical_cast<std::string>((*this)(index));
}

/**
 * Create a set of bin boundaries from the centre point values
 * @returns A vector of bin boundaries
 */
std::vector<double> NumericAxis::createBinBoundaries() const
{
  const std::size_t npoints = length();
  if( npoints < 2 ) throw std::runtime_error("Fewer than two points on axis, cannot create bins.");
  std::vector<double> boundaries(npoints + 1);
  const NumericAxis & thisObject(*this);
  for( size_t i = 0; i < npoints - 1; ++i )
  {
    boundaries[i+1] = 0.5*(thisObject(i) + thisObject(i+1));
  }
  boundaries[0] = m_values.front() - (boundaries[1] - m_values.front());
  boundaries[npoints] = m_values.back() + (m_values.back() - boundaries[npoints - 1]);
  return boundaries;
}

/** Get a const reference to the vector of values in this axis
 *
 * @return the values vector
 */
const std::vector<double> &  NumericAxis::getValues() const
{
  return m_values;
}

} // namespace API
} // namespace Mantid
