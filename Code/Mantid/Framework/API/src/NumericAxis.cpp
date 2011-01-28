//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Exception.h"

#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace API
{

/// Constructor
NumericAxis::NumericAxis(const int& length): Axis()
{
  m_values.resize(length);
}

/** Virtual constructor
 *  @param parentWorkspace :: The workspace is not used in this implementation
 *  @returns A pointer to a copy of the NumericAxis on which the method is called
 */
Axis* NumericAxis::clone(const MatrixWorkspace* const parentWorkspace)
{
  (void) parentWorkspace; //Avoid compiler warning
  return new NumericAxis(*this);
}

/** Get the axis value at the position given
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex Needed for the subclass (RefAxis) method, but ignored (and defaulted) here
 *  @return The value of the axis as a double
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
double NumericAxis::operator()(const int& index, const int& verticalIndex) const
{
  (void) verticalIndex; //Avoid compiler warning
  if (index < 0 || index >= length())
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
void NumericAxis::setValue(const int& index, const double& value)
{
  if (index < 0 || index >= length())
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
std::string NumericAxis::label(const int& index)const
{
  return boost::lexical_cast<std::string>((*this)(index));
}

} // namespace API
} // namespace Mantid
