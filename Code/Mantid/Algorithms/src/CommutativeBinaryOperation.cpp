//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include "MantidAlgorithms/CommutativeBinaryOperation.h"
#include "MantidAPI/WorkspaceIterator.h"

namespace Mantid
{
  namespace Algorithms
  {
    // Get a reference to the logger
    Kernel::Logger& CommutativeBinaryOperation::g_log = Kernel::Logger::get("CommutativeBinaryOperation");

    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
    * In order to be size compatible then the larger workspace
    * must divide be the size of the smaller workspace leaving no remainder
    * @param rhs the first workspace to compare
    * @param lhs the second workspace to compare
    * @retval true The two workspaces are size compatible
    * @retval false The two workspaces are NOT size compatible
    */
    const bool CommutativeBinaryOperation::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr rhs,const API::MatrixWorkspace_const_sptr lhs) const
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

    /** Creates a suitable output workspace for a binary operatiion based on the two input workspaces
    * @param rhs the first workspace to compare
    * @param lhs the second workspace to compare
    * @returns a pointer to a new zero filled workspace the same type and size as the larger of the two input workspaces.
    */
    API::MatrixWorkspace_sptr CommutativeBinaryOperation::createOutputWorkspace(const API::MatrixWorkspace_const_sptr rhs, const API::MatrixWorkspace_const_sptr lhs) const
    {
      //get the largest workspace
      const API::MatrixWorkspace_const_sptr wsLarger = (lhs->size() > rhs->size()) ? lhs : rhs;
      //get the smallest workspace
      const API::MatrixWorkspace_const_sptr wsSmaller = (lhs->size() > rhs->size()) ? rhs : lhs;

      return BinaryOperation::createOutputWorkspace(wsLarger,wsSmaller);
    }

  }
}
