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
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        // Get references to the input Y's
        const double& leftY = lhsY[j];
        const double& rightY = rhsY[j];

        // Calculate result and store in local variable to avoid overwriting original data if
        // output workspace is same as one of the input ones
        const double Y = leftY/rightY;

        if (fabs(rightY)>1.0e-12 && fabs(Y)>1.0e-12)
        {
          //  gaussian errors
          // (Sa/a)2 + (Sb/b)2 = (Sc/c)2
          //  So after taking proportions, squaring, summing,
          //  and taking the square root, you get a proportional error to the product c.
          //  Multiply that proportional error by c to get the actual standard deviation Sc.
          const double lhsFactor = (lhsE[j]<1.0e-12|| fabs(leftY)<1.0e-12) ? 0.0 : pow((lhsE[j]/leftY),2);
          const double rhsFactor = rhsE[j]<1.0e-12 ? 0.0 : pow((rhsE[j]/rightY),2);
          EOut[j] = std::abs(Y) * sqrt(lhsFactor+rhsFactor);
        }

        // Now store the result
        YOut[j] = Y;
      }
    }

    void Divide::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      // Do the right-hand part of the error calculation just once
      const double rhsFactor = (rhsE<1.0e-12) ? 0.0 : pow((rhsE/rhsY),2);
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        // Get reference to input Y
        const double& leftY = lhsY[j];
        // Calculate result into local variable
        const double Y = leftY/rhsY;

        if (fabs(Y)>1.0e-12)
        {
          const double lhsFactor = (lhsE[j]<1.0e-12 || fabs(leftY)<1.0e-12) ? 0.0 : pow((lhsE[j]/leftY),2);
          EOut[j] = std::abs(Y) * sqrt(lhsFactor+rhsFactor);
        }

        // Copy result in
        YOut[j] = Y;
      }
    }

  } // namespace Algorithms
} // namespace Mantid
