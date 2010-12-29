//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Multiply.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Multiply)

    void Multiply::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                          const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        // Get references to the input Y's
        const double& leftY = lhsY[j];
        const double& rightY = rhsY[j];

         // error multiplying two uncorrelated numbers, re-arrange so that you don't get infinity if leftY or rightY == 0
        // (Sa/a)2 + (Sb/b)2 = (Sc/c)2
        // (Sc)2 = (Sa c/a)2 + (Sb c/b)2 
        //       = (Sa b)2 + (Sb a)2 
        EOut[j] = sqrt(pow(lhsE[j]*rightY, 2) + pow(rhsE[j]*leftY, 2));

        // Copy the result last in case one of the input workspaces is also any output
        YOut[j] = leftY*rightY;
      }
    }

    void Multiply::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                          const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        // Get reference to input Y
        const double& leftY = lhsY[j];

        // see comment in the function above for the error formula
        EOut[j] = sqrt(pow(lhsE[j]*rhsY, 2) + pow(rhsE*leftY, 2));

        // Copy the result last in case one of the input workspaces is also any output
        YOut[j] = leftY*rhsY;
      }
    }






    // ===================================== EVENT LIST BINARY OPERATIONS ==========================================
    /** Carries out the binary operation IN-PLACE on a single EventList,
     * with another EventList as the right-hand operand.
     *
     *  @param lhs Reference to the EventList that will be modified in place.
     *  @param rhs Const reference to the EventList on the right hand side.
     */
    void Multiply::performEventBinaryOperation(DataObjects::EventList & lhs,
        const DataObjects::EventList & rhs)
    {
      // We must histogram the rhs event list to multiply.
      MantidVec rhsY, rhsE;
      rhs.generateHistogram( rhs.dataX(), rhsY, rhsE);
      lhs.multiply(rhs.dataX(), rhsY, rhsE);
    }

    /** Carries out the binary operation IN-PLACE on a single EventList,
     * with another (histogrammed) spectrum as the right-hand operand.
     *
     *  @param lhs Reference to the EventList that will be modified in place.
     *  @param rhsX The vector of rhs X bin boundaries
     *  @param rhsY The vector of rhs data values
     *  @param rhsE The vector of rhs error values
     */
    void Multiply::performEventBinaryOperation(DataObjects::EventList & lhs,
        const MantidVec& rhsX, const MantidVec& rhsY, const MantidVec& rhsE)
    {
      // Multiply is implemented at the EventList level.
      lhs.multiply(rhsX, rhsY, rhsE);
    }

    /** Carries out the binary operation IN-PLACE on a single EventList,
     * with a single (double) value as the right-hand operand.
     * Performs the multiplication by a scalar (with error)
     *
     *  @param lhs Reference to the EventList that will be modified in place.
     *  @param rhsY The rhs data value
     *  @param rhsE The rhs error value
     */
    void Multiply::performEventBinaryOperation(DataObjects::EventList & lhs,
        const double& rhsY, const double& rhsE)
    {
      // Multiply is implemented at the EventList level.
      lhs.multiply(rhsY, rhsE);
    }



    //---------------------------------------------------------------------------------------------
    /** Check what operation will be needed in order to apply the operation
     * to these two types of workspaces. This function must be overridden
     * and checked against all 9 possible combinations.
     *
     * Must set: m_matchXSize, m_flipSides, m_keepEventWorkspace
     */
    void Multiply::checkRequirements()
    {
      if (m_elhs)
      {
        // The lhs workspace is an EventWorkspace. It can be multiplied while keeping event-ishness
        //Output will be EW
        m_keepEventWorkspace = true;
        //Histogram sizes need not match
        m_matchXSize = false;
        //For now, only the lhs can be event workspace. So don't flip
        //TODO: HANDLE
        m_flipSides = false;
      }
      else
      {
        // either or both workspace are "other"
        // Use the default behaviour
        BinaryOperation::checkRequirements();
      }
    }



    //--------------------------------------------------------------------------------------------
    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
     *  In order to be size compatible then the larger workspace
     *  must divide be the size of the smaller workspace leaving no remainder
     *
     *  @param lhs the first workspace to compare
     *  @param rhs the second workspace to compare
     *  @retval true The two workspaces are size compatible
     *  @retval false The two workspaces are NOT size compatible
     */
    bool Multiply::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      if (!m_keepEventWorkspace)
      {
        // Fallback on the default checks
        return BinaryOperation::checkSizeCompatibility(lhs, rhs);
      }
      else
      {
        // --- Check for event workspaces - different than workspaces 2D! ---

        // A SingleValueWorkspace on the right matches anything
        WorkspaceSingleValue_const_sptr rhs_single = boost::dynamic_pointer_cast<const WorkspaceSingleValue>(rhs);
        if (rhs_single) return true;

        // A SingleValueWorkspace on the left only matches if rhs was single value too. Why are you using mantid to do simple math?!?
        WorkspaceSingleValue_const_sptr lhs_single = boost::dynamic_pointer_cast<const WorkspaceSingleValue>(lhs);
        if (lhs_single) return false;

        // RHS only has one value (1D vertical), so the number of histograms needs to match.
        // Each lhs spectrum will be divided by that scalar
        if ( rhs->blocksize() == 1 && lhs->getNumberHistograms() == rhs->getNumberHistograms() ) return true;

        // We don't need to check for matching bins. Yay events!

        const int rhsSpec = rhs->getNumberHistograms();

        // If the rhs has a single spectrum, then we can divide. The block size does NOT need to match,
        if (rhsSpec == 1) return true;

        // Otherwise, the number of histograms needs to match, but the block size of each does NOT need to match.
        return ( lhs->getNumberHistograms() == rhs->getNumberHistograms() );
      }
    }


  } // namespace Algorithms
} // namespace Mantid
