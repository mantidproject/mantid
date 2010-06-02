//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PolynomialCorrection.h"
#include "MantidKernel/ArrayProperty.h"
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{
  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(PolynomialCorrection)

  void PolynomialCorrection::defineProperties()
  {
    // We need an array property for the coefficients of the polynomial: C0 + C1*x + C2*x*x + ....
    declareProperty(new ArrayProperty<double>("Coefficients",new MandatoryValidator<std::vector<double> >));
  }
  
  void PolynomialCorrection::retrieveProperties()
  {
    // So this will be m_coeffs[0] + m_coeffs[1]*x + m_coeffs[2]*x^2 + ...
    m_coeffs = getProperty("Coefficients");
    m_polySize = m_coeffs.size();
  }
  
  void PolynomialCorrection::performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut)
  {
    // The first value of the coefficients vector is for the zeroth power of X.
    double factor = m_coeffs[0];
    double xPow = 1.0;
    // Build up the polynomial in ascending powers of x
    for (std::vector<double>::size_type i = 1; i < m_polySize; ++i)
    {
      xPow *= XIn;
      factor += m_coeffs[i]*xPow;
    }
    
    // Multiply the data and error by the correction factor
    YOut = YIn*factor;
    EOut = EIn*std::abs(factor);
  }

} // namespace Algorithms
} // namespace Mantid
