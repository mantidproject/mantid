//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CommutativeBinaryOperation.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
     * In order to be size compatible then the larger workspace
     * must divide be the size of the smaller workspace leaving no remainder
     * @param lhs the workspace treated as the lhs to compare
     * @param rhs the workspace treated as the rhs to compare
     * @retval true The two workspaces are size compatible
     * @retval false The two workspaces are NOT size compatible
     */
    bool CommutativeBinaryOperation::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      // Don't allow this for EventWorkspaces. See for instance Multiply::checkSizeCompatability
      if( boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(lhs) || 
	  boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(rhs) )
      {
	//call the base routine
	return BinaryOperation::checkSizeCompatibility(lhs,rhs);
      }

      //get the largest workspace
      API::MatrixWorkspace_const_sptr wsLarger;
      API::MatrixWorkspace_const_sptr wsSmaller;
      if (rhs->size() > lhs->size())
      {
        wsLarger = rhs;
        wsSmaller = lhs;
      }
      else
      {
        wsLarger = lhs;
        wsSmaller = rhs;
      }
      //call the base routine
      return BinaryOperation::checkSizeCompatibility(wsLarger,wsSmaller);
    }

  }
}
