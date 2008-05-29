//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace API
{

/** Constructor
 *  @param parentWorkspace A pointer to the workspace that holds this axis
 */
RefAxis::RefAxis(const Workspace* const parentWorkspace) : 
  Axis(AxisType::Numeric, 0),
  m_parentWS(parentWorkspace)
{}

/** Private, specialised copy constructor. Needed because it's necessary to pass in
 *  a pointer to the parent of the new workspace, rather than having the copy point
 *  to the parent of the copied axis.
 *  @param right The axis to copy
 *  @param parentWorkspace A pointer to the parent workspace of the new axis
 */
RefAxis::RefAxis(const RefAxis& right, const Workspace* const parentWorkspace) :
  Axis(right), m_parentWS(parentWorkspace)
{}

RefAxis::~RefAxis()
{}

/** Virtual constructor
 *  @param parentWorkspace A pointer to the workspace that will hold the new axis
 *  @return A pointer to a copy of the Axis on which the method is called
 */
Axis* RefAxis::clone(const Workspace* const parentWorkspace)
{ 
  return new RefAxis(*this, parentWorkspace); 
}

/** Get the axis value at the position given. In this case, the values are held in the
 *  X vectors of the workspace itself.
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex The position along the orthogonal axis
 *  @return The value of the axis as a double
 *  @throw  IndexError If either index requested is not in the range of this axis
 */
const double RefAxis::operator()(const int index, const int verticalIndex) const
{
  return m_parentWS->dataX(verticalIndex)[index];
}

} // namespace API
} // namespace Mantid
