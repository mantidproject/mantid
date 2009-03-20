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
      
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        if (fabs(lhsE[j])>1e-7)
          EOut[j] = YOut[j]*sqrt(pow((lhsE[j]/lhsY[j]),2)+pow((rhsE[j]/rhsY[j]),2));
      }
    }

    void Divide::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      std::transform(lhsY.begin(),lhsY.end(),YOut.begin(),std::bind2nd(std::divides<double>(),rhsY));
      
      const double rhsFactor = rhsE ? pow((rhsE/rhsY),2) : 0.0;
      const int bins = lhsE.size();
      for (int j=0; j<bins; ++j)
      {
        if (fabs(lhsE[j])>1e-7)
          EOut[j] = YOut[j]*sqrt(pow((lhsE[j]/lhsY[j]),2)+rhsFactor);
      }
    }
    
  }
}
