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

    // Get a reference to the logger
    Logger& Divide::g_log = Logger::get("Divide");

    void Divide::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      std::transform(lhsY.begin(),lhsY.end(),rhsY.begin(),YOut.begin(),std::divides<double>());
      
      //  gaussian errors
      // (Sa/a)2 + (Sb/b)2 = (Sc/c)2
      //  So after taking proportions, squaring, summing,
      //  and taking the square root, you get a proportional error to the product c.
      //  Multiply that proportional error by c to get the actual standard deviation Sc.
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        const double denom = rhsY[j];
        if (fabs(denom)<1.0e-12) continue;
        const double Y = YOut[j];
        if (fabs(Y)<1.0e-12) continue;
        const double lhsFactor = (lhsE[j]<1.0e-12|| fabs(lhsY[j])<1.0e-12) ? 0.0 : pow((lhsE[j]/lhsY[j]),2);
        const double rhsFactor = rhsE[j]<1.0e-12 ? 0.0 : pow((rhsE[j]/denom),2);
        EOut[j] = Y * sqrt(lhsFactor+rhsFactor);
      }
    }

    void Divide::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      std::transform(lhsY.begin(),lhsY.end(),YOut.begin(),std::bind2nd(std::divides<double>(),rhsY));
      
      if (fabs(rhsY)<1.0e-12) return;
      const double rhsFactor = (rhsE<1.0e-12) ? 0.0 : pow((rhsE/rhsY),2);
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        const double Y = YOut[j];
        if (fabs(Y)<1.0e-12) continue;
        const double lhsFactor = (lhsE[j]<1.0e-12 || fabs(lhsY[j])<1.0e-12) ? 0.0 : pow((lhsE[j]/lhsY[j]),2);
        EOut[j] = Y * sqrt(lhsFactor+rhsFactor);
      }
    }
    
  }
}
