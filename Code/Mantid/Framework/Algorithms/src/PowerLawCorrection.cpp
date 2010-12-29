//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PowerLawCorrection.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{
  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(PowerLawCorrection)

  void PowerLawCorrection::defineProperties()
  {
    // We need an array property for the coefficients of the PowerLaw: C0*X^C1
    declareProperty("C0",1.0,"The value by which the entire calculation is multiplied");
    declareProperty("C1",1.0,"The power by which the x value is raised");
  }
  
  void PowerLawCorrection::retrieveProperties()
  {
    m_c0 = getProperty("C0");
    m_c1 = getProperty("C1");
  }
  
  void PowerLawCorrection::performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut)
  {
    double factor = m_c0*pow(XIn,m_c1);
    // Multiply the data and error by the correction factor
    YOut = YIn*factor;
    EOut = EIn*factor;
  }

} // namespace Algorithms
} // namespace Mantid
