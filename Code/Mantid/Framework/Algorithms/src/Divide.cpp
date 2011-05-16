//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Divide.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Divide)
    
    /// Sets documentation strings for this algorithm
    void Divide::initDocs()
    {
      this->setWikiSummary("The Divide algorithm will divide the data values and calculate the corresponding [[Error Values|error values]] of two compatible workspaces.  {{BinaryOperation|verb=divided|prep=by|symbol=<math>\\div</math>}} ");
      this->setOptionalMessage("The Divide algorithm will divide the data values and calculate the corresponding error values of two compatible workspaces.");
    }
    

    void Divide::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning

      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        // Get references to the input Y's
        const double leftY = lhsY[j];
        const double rightY = rhsY[j];

        //  error dividing two uncorrelated numbers, re-arrange so that you don't get infinity if leftY==0 (when rightY=0 the Y value and the result will both be infinity)
        // (Sa/a)2 + (Sb/b)2 = (Sc/c)2
        // (Sa c/a)2 + (Sb c/b)2 = (Sc)2
        // = (Sa 1/b)2 + (Sb (a/b2))2
        // (Sc)2 = (1/b)2( (Sa)2 + (Sb a/b)2 )
        EOut[j] = sqrt( pow(lhsE[j], 2)+pow( leftY*rhsE[j]/rightY, 2) )/rightY;

        // Copy the result last in case one of the input workspaces is also any output
        YOut[j] = leftY/rightY;;
      }
    }

    void Divide::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                        const double rhsY, const double rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning
      // Do the right-hand part of the error calculation just once
      const double rhsFactor = pow(rhsE/rhsY,2);
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        // Get reference to input Y
        const double leftY = lhsY[j];

        // see comment in the function above for the error formula
        EOut[j] = sqrt( pow(lhsE[j], 2)+pow( leftY, 2)*rhsFactor )/rhsY;
        // Copy the result last in case one of the input workspaces is also any output
        YOut[j] = leftY/rhsY;
      }
    }

    void Divide::setOutputUnits(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
    {
      if ( rhs->YUnit().empty() || !WorkspaceHelpers::matchingBins(lhs,rhs,true) )
      {
        // Do nothing
      }
      // If the Y units match, then the output will be a distribution and will be dimensionless
      else if ( lhs->YUnit() == rhs->YUnit() && rhs->blocksize() > 1 )
      {
        out->setYUnit("");
        out->isDistribution(true);
      }
      // Else we need to set the unit that results from the division
      else
      {
        if ( ! lhs->YUnit().empty() ) out->setYUnit(lhs->YUnit() + "/" + rhs->YUnit());
        else out->setYUnit("1/" + rhs->YUnit());
      }
    }




    // ===================================== EVENT LIST BINARY OPERATIONS ==========================================
    /** Carries out the binary operation IN-PLACE on a single EventList,
     * with another EventList as the right-hand operand.
     *
     *  @param lhs :: Reference to the EventList that will be modified in place.
     *  @param rhs :: Const reference to the EventList on the right hand side.
     */
    void Divide::performEventBinaryOperation(DataObjects::EventList & lhs,
        const DataObjects::EventList & rhs)
    {
      // We must histogram the rhs event list to divide.
      MantidVec rhsY, rhsE;
      rhs.generateHistogram( rhs.dataX(), rhsY, rhsE);
      lhs.divide(rhs.dataX(), rhsY, rhsE);
    }

    /** Carries out the binary operation IN-PLACE on a single EventList,
     * with another (histogrammed) spectrum as the right-hand operand.
     *
     *  @param lhs :: Reference to the EventList that will be modified in place.
     *  @param rhsX :: The vector of rhs X bin boundaries
     *  @param rhsY :: The vector of rhs data values
     *  @param rhsE :: The vector of rhs error values
     */
    void Divide::performEventBinaryOperation(DataObjects::EventList & lhs,
        const MantidVec& rhsX, const MantidVec& rhsY, const MantidVec& rhsE)
    {
      // Divide is implemented at the EventList level.
      lhs.divide(rhsX, rhsY, rhsE);
    }

    /** Carries out the binary operation IN-PLACE on a single EventList,
     * with a single (double) value as the right-hand operand.
     * Performs the multiplication by a scalar (with error)
     *
     *  @param lhs :: Reference to the EventList that will be modified in place.
     *  @param rhsY :: The rhs data value
     *  @param rhsE :: The rhs error value
     */
    void Divide::performEventBinaryOperation(DataObjects::EventList & lhs,
        const double& rhsY, const double& rhsE)
    {
      // Multiply is implemented at the EventList level.
      lhs.divide(rhsY, rhsE);
    }



    //---------------------------------------------------------------------------------------------
    /** Check what operation will be needed in order to apply the operation
     * to these two types of workspaces. This function must be overridden
     * and checked against all 9 possible combinations.
     *
     * Must set: m_matchXSize, m_flipSides, m_keepEventWorkspace
     */
    void Divide::checkRequirements()
    {
      if (m_elhs)
      {
        // The lhs workspace is an EventWorkspace. It can be divided while keeping event-ishness
        // Output will be EW
        m_keepEventWorkspace = true;
        //Histogram sizes need not match
        m_matchXSize = false;
      }
      else
      {
        m_keepEventWorkspace = false;
        m_matchXSize = true;
      }

      // Division is not commutative = you can't flip sides.
      m_flipSides = false;
      // The RHS operand will be histogrammed first.
      m_useHistogramForRhsEventWorkspace = true;
    }


    //--------------------------------------------------------------------------------------------
    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
     *  In order to be size compatible then the larger workspace
     *  must divide be the size of the smaller workspace leaving no remainder
     *
     *  @param lhs :: the first workspace to compare
     *  @param rhs :: the second workspace to compare
     *  @retval true The two workspaces are size compatible
     *  @retval false The two workspaces are NOT size compatible
     */
    bool Divide::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      // --- Check for event workspaces - different than workspaces 2D! ---

      // A SingleValueWorkspace on the right matches anything
      if (rhs->size()==1) return true;

      // A SingleValueWorkspace on the left only matches if rhs was single value too. Why are you using mantid to do simple math?!?
      if (lhs->size()==1) return false;

      // If RHS only has one value (1D vertical), the number of histograms needs to match.
      // Each lhs spectrum will be divided by that scalar
      //std::cout << "rhs->blocksize() " << rhs->blocksize() << std::endl;
      if ( rhs->blocksize() == 1 && lhs->getNumberHistograms() == rhs->getNumberHistograms() ) return true;

      if (m_matchXSize)
      {
        // Past this point, for a 2D WS operation, we require the X arrays to match. Note this only checks the first spectrum
        if ( !WorkspaceHelpers::matchingBins(lhs,rhs,true) ) return false;
      }

      // We don't need to check for matching bins for events. Yay events!
      const size_t rhsSpec = rhs->getNumberHistograms();

      // If the rhs has a single spectrum, then we can divide. The block size does NOT need to match,
      if (rhsSpec == 1) return true;

      // Are we allowing the division by different # of spectra, using detector IDs to match up?
      if (m_AllowDifferentNumberSpectra)
      {
        return true;
      }

      // Otherwise, the number of histograms needs to match, but the block size of each does NOT need to match.
      return ( lhs->getNumberHistograms() == rhs->getNumberHistograms() );

    }


  } // namespace Algorithms
} // namespace Mantid
