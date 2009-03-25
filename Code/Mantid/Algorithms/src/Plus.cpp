//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Plus.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Plus)

    // Get a reference to the logger
    Logger& Plus::g_log = Logger::get("Plus");

    void Plus::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      std::transform(lhsY.begin(),lhsY.end(),rhsY.begin(),YOut.begin(),std::plus<double>());
      std::transform(lhsE.begin(),lhsE.end(),rhsE.begin(),EOut.begin(),SumGaussError<double>());
    }

    void Plus::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                       const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      std::transform(lhsY.begin(),lhsY.end(),YOut.begin(),std::bind2nd(std::plus<double>(),rhsY));
      // Only do E if non-zero, otherwise just copy
      if (rhsE > 1.0e-12)
        std::transform(lhsE.begin(),lhsE.end(),EOut.begin(),std::bind2nd(SumGaussError<double>(),rhsE));
      else
        EOut = lhsE;
    }
    
    const bool Plus::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      if ( lhs->size() > 1 && rhs->size() > 1 && lhs->YUnit() != rhs->YUnit() )
      {
        g_log.error("The two workspace are not compatible because they have different units for the data (Y).");
        return false;
      }
      if ( lhs->isDistribution() != rhs->isDistribution() )
      {
        g_log.error("The two workspace are not compatible because one is flagged as a distribution.");
        return false;
      }
      
      return BinaryOperation::checkCompatibility(lhs,rhs);
    }

  }
}
