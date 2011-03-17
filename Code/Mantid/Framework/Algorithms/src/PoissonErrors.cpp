//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PoissonErrors.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(PoissonErrors)
    
    /// Sets documentation strings for this algorithm
    void PoissonErrors::initDocs()
    {
      this->setWikiSummary("Calculates the gaussian approxiamtion of Poisson error based on a matching workspace containing the original counts. ");
      this->setOptionalMessage("Calculates the gaussian approxiamtion of Poisson error based on a matching workspace containing the original counts.");
    }
    

    /** Performs a simple check to see if the sizes of two workspaces are identically sized
    * @param lhs :: the first workspace to compare
    * @param rhs :: the second workspace to compare
    * @retval true The two workspaces are size compatible
    * @retval false The two workspaces are NOT size compatible
    */
    bool PoissonErrors::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      //in order to be size compatible then the workspaces must be identically sized
      return (lhs->size() == rhs->size());
    }

    void PoissonErrors::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                               const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning
      // Just copy over the lhs data
      YOut = lhsY;
      // Now make the fractional error the same as it was on the rhs
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
         const double fractional = rhsY[j] ? rhsE[j]/rhsY[j] : 0.0;
         EOut[j] = fractional*lhsY[j];
      }
    }
    
    void PoissonErrors::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                               const double rhsY, const double rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsE; (void) lhsX; //Avoid compiler warning

      assert( lhsX.size() == 1 );
      // If we get here we've got two single column workspaces so it's easy.
      YOut[0] = lhsY[0];
      
      const double fractional = rhsY ? rhsE/rhsY : 0.0;
      EOut[0] = fractional*lhsY[0];
    }
    
  }
}
