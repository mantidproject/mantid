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
     * @param rhs the first workspace to compare
     * @param lhs the second workspace to compare
     * @retval true The two workspaces are size compatible
     * @retval false The two workspaces are NOT size compatible
     */
    bool CommutativeBinaryOperation::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr rhs,const API::MatrixWorkspace_const_sptr lhs) const
    {
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
