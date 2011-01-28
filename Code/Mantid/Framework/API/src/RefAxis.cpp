//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace API
{

/** Constructor
 *  @param length :: The length of this axis
 *  @param parentWorkspace :: A pointer to the workspace that holds this axis
 */
RefAxis::RefAxis(const int& length, const MatrixWorkspace* const parentWorkspace) : 
  NumericAxis(length),  m_parentWS(parentWorkspace)
{
  m_size = length;
}

/** Private, specialised copy constructor. Needed because it's necessary to pass in
 *  a pointer to the parent of the new workspace, rather than having the copy point
 *  to the parent of the copied axis.
 *  @param right :: The axis to copy
 *  @param parentWorkspace :: A pointer to the parent workspace of the new axis
 */
RefAxis::RefAxis(const RefAxis& right, const MatrixWorkspace* const parentWorkspace) :
NumericAxis(right), m_parentWS(parentWorkspace), m_size(right.m_size)
{}

RefAxis::~RefAxis()
{}

/** Virtual constructor
 *  @param parentWorkspace :: A pointer to the workspace that will hold the new axis
 *  @return A pointer to a copy of the Axis on which the method is called
 */
Axis* RefAxis::clone(const MatrixWorkspace* const parentWorkspace)
{ 
  return new RefAxis(*this, parentWorkspace); 
}

/** Get the axis value at the position given. In this case, the values are held in the
 *  X vectors of the workspace itself.
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex The position along the orthogonal axis
 *  @return The value of the axis as a double
 *  @throw  IndexError If 'index' is not in the range of this axis
 *  @throw  std::range_error If 'verticalIndex' is not in the range of the parent workspace
 */
double RefAxis::operator()(const int& index, const int& verticalIndex) const
{
  if (index < 0 || index >= m_size)
  {
    throw Kernel::Exception::IndexError(index, m_size-1, "Axis: Index out of range.");
  }

  return m_parentWS->dataX(verticalIndex)[index];
}

/// Method not available for RefAxis. Will always throw.
void RefAxis::setValue(const int& index, const double& value)
{
  (void) index; (void) value; //Avoid compiler warning
  throw std::domain_error("This method cannot be used on a RefAxis.");
}

/** Check if two axis defined as spectra or numeric axis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true is self and second axis are equal
 */
bool RefAxis::operator==(const Axis& axis2) const
{
	if (length()!=axis2.length())
  {
		return false;
  }
  const RefAxis* ra2 = dynamic_cast<const RefAxis*>(&axis2);
  if (!ra2)
  {
    return false;
  }
	return true;
}

} // namespace API
} // namespace Mantid
