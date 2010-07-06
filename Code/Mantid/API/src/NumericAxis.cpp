//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Exception.h"

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
 *  @return A pointer to a copy of the NumericAxis on which the method is called
 */
Axis* NumericAxis::clone(const MatrixWorkspace* const)
{
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
  if (index < 0 || index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "NumericAxis: Index out of range.");
  }

  return m_values[index];
}

/** Sets the axis value at a given position
 *  @param index The position along the axis for which to set the value
 *  @param value The new value
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
 *  @param axis2 Reference to the axis to compare to
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

} // namespace API
} // namespace Mantid
