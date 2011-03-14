//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Minus.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Minus)
    
    /// Sets documentation strings for this algorithm
    void Minus::initDocs()
    {
      this->setWikiSummary("The Minus algorithm will subtract the data values and calculate the corresponding [[Error Values|error values]] for two compatible workspaces.  {{BinaryOperation|verb=subtracted|prep=from|symbol=<math>-</math>}} ");
      this->setOptionalMessage("The Minus algorithm will subtract the data values and calculate the corresponding error values for two compatible workspaces.");
    }
    

    void Minus::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning
      std::transform(lhsY.begin(),lhsY.end(),rhsY.begin(),YOut.begin(),std::minus<double>());
      std::transform(lhsE.begin(),lhsE.end(),rhsE.begin(),EOut.begin(),VectorHelper::SumGaussError<double>());
    }

    void Minus::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning
      std::transform(lhsY.begin(),lhsY.end(),YOut.begin(),std::bind2nd(std::minus<double>(),rhsY));
      // Only do E if non-zero, otherwise just copy
      if (rhsE != 0)
        std::transform(lhsE.begin(),lhsE.end(),EOut.begin(),std::bind2nd(VectorHelper::SumGaussError<double>(),rhsE));
      else
        EOut = lhsE;
    }
    


    // ===================================== EVENT LIST BINARY OPERATIONS ==========================================
    /** Carries out the binary operation IN-PLACE on a single EventList,
     * with another EventList as the right-hand operand.
     * The event lists simply get appended, with the rhs being negatively weighted.
     *
     *  @param lhs :: Reference to the EventList that will be modified in place.
     *  @param rhs :: Const reference to the EventList on the right hand side.
     */
    void Minus::performEventBinaryOperation(DataObjects::EventList & lhs,
        const DataObjects::EventList & rhs)
    {
      //Easy, no? :) - This appends the event lists, with the rhs being negatively weighted.
      lhs -= rhs;
    }

    /** Carries out the binary operation IN-PLACE on a single EventList,
     * with another (histogrammed) spectrum as the right-hand operand.
     *
     *  @param lhs :: Reference to the EventList that will be modified in place.
     *  @param rhsX :: The vector of rhs X bin boundaries
     *  @param rhsY :: The vector of rhs data values
     *  @param rhsE :: The vector of rhs error values
     */
    void Minus::performEventBinaryOperation(DataObjects::EventList & lhs,
        const MantidVec& rhsX, const MantidVec& rhsY, const MantidVec& rhsE)
    {
      (void) lhs;  //Avoid compiler warnings
      (void) rhsX; (void) rhsY; (void) rhsE;
      throw Exception::NotImplementedError("Plus::performEventBinaryOperation() cannot subtract a histogram from an event list in an EventWorkspace. Try switching to a Workspace2D before using Minus.");
    }

    /** Carries out the binary operation IN-PLACE on a single EventList,
     * with a single (double) value as the right-hand operand.
     * THROWS since it is not possible to add a value to an event list.
     *
     *  @param lhs :: Reference to the EventList that will be modified in place.
     *  @param rhsY :: The rhs data value
     *  @param rhsE :: The rhs error value
     */
    void Minus::performEventBinaryOperation(DataObjects::EventList & lhs,
        const double& rhsY, const double& rhsE)
    {
      (void) lhs;  //Avoid compiler warnings
      (void) rhsY; (void) rhsE;
      throw Exception::NotImplementedError("Plus::performEventBinaryOperation() cannot subtract a number from an event list in an EventWorkspace. Try switching to a Workspace2D before using Minus.");
    }



    //---------------------------------------------------------------------------------------------
    /** Check what operation will be needed in order to apply the operation
     * to these two types of workspaces. This function must be overridden
     * and checked against all 9 possible combinations.
     *
     * Must set: m_matchXSize, m_flipSides, m_keepEventWorkspace
     */
    void Minus::checkRequirements()
    {
      if (m_erhs && m_elhs)
      {
        //Two EventWorkspaces! They can be concatenated.
        //Output will be EW
        m_keepEventWorkspace = true;
        //Histogram sizes need not match
        m_matchXSize = false;
        //Can't flip - this is non-commutative
        m_flipSides = false;
        // Special case for plus/minus: if there is only one bin on the RHS, use the 2D method (appending event lists)
        // so that the single bin is not treated as a scalar
        m_do2D_even_for_SingleColumn_on_rhs = true;
      }
      else
      {
        // either or both workspace are "other"
        // Use the default behaviour
        BinaryOperation::checkRequirements();
      }
    }


    //---------------------------------------------------------------------------------------------
    /**
     *  Return true if the units and distribution-type of the workspaces make them compatible
     *  @param lhs :: first workspace to check for compatibility
     *  @param rhs :: second workspace to check for compatibility
     *  @return workspace unit compatibility flag
     */
    bool Minus::checkUnitCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      if ( lhs->size() > 1 && rhs->size() > 1 )
      {
        if ( lhs->YUnit() != rhs->YUnit() )
        {
          g_log.error("The two workspaces are not compatible because they have different units for the data (Y).");
          return false;
        }
        if ( lhs->isDistribution() != rhs->isDistribution() )
        {
          g_log.error("The two workspaces are not compatible because one is flagged as a distribution.");
          return false;
        }
      }
      return true;
    }

    //---------------------------------------------------------------------------------------------
    /**
     *  Check the given workspaces for unit, distribution and binary operation
     *  compatibility. Return is true is the workspaces are compatible.
     *  @param lhs :: first workspace to check for compatibility
     *  @param rhs :: second workspace to check for compatibility
     *  @return workspace compatibility flag
     */
    bool Minus::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      if (!checkUnitCompatibility(lhs, rhs))
        return false;

      //Keep checking more generally.
      return BinaryOperation::checkCompatibility(lhs,rhs);
    }


    //--------------------------------------------------------------------------------------------
    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
     *  In order to be size compatible then the larger workspace
     *  must divide be the size of the smaller workspace leaving no remainder
     *  @param lhs :: the first workspace to compare
     *  @param rhs :: the second workspace to compare
     *  @retval true The two workspaces are size compatible
     *  @retval false The two workspaces are NOT size compatible
     */
    bool Minus::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      if (m_erhs && m_elhs)
      {
        return ( lhs->getNumberHistograms() == rhs->getNumberHistograms() );
      }
      else
        return BinaryOperation::checkSizeCompatibility(lhs, rhs);
    }



  }
}
