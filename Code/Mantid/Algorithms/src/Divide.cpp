//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Divide.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Divide)

    void Divide::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning

      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        // Get references to the input Y's
        const double& leftY = lhsY[j];
        const double& rightY = rhsY[j];

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
                                       const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning
      // Do the right-hand part of the error calculation just once
      const double rhsFactor = pow(rhsE/rhsY,2);
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        // Get reference to input Y
        const double& leftY = lhsY[j];

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

  } // namespace Algorithms
} // namespace Mantid
