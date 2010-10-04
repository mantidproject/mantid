//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Multiply.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

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

  } // namespace Algorithms
} // namespace Mantid
