/*WIKI* 


Corrects the data and error values on a workspace by the value of a polynomial function:
:<math> {\rm C0} + {\rm C1} x + {\rm C2} x^2 + ... </math>
which is evaluated at the ''x'' value of each data point (using the mid-point of the bin as the ''x'' value for 
histogram data. The data and error values are multiplied or divided by the value of this function.
The order of the polynomial is determined by the length of the Coefficients property, which can be of any length.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PolynomialCorrection.h"
#include "MantidKernel/ArrayProperty.h"
#include <cmath>
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{
  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(PolynomialCorrection)
  
  /// Sets documentation strings for this algorithm
  void PolynomialCorrection::initDocs()
  {
    this->setWikiSummary("Corrects the data in a workspace by the value of a polynomial function which is evaluated at the X value of each data point. ");
    this->setOptionalMessage("Corrects the data in a workspace by the value of a polynomial function which is evaluated at the X value of each data point.");
  }
  

  void PolynomialCorrection::defineProperties()
  {
    // We need an array property for the coefficients of the polynomial: C0 + C1*x + C2*x*x + ....
    declareProperty(new ArrayProperty<double>("Coefficients",boost::make_shared<MandatoryValidator<std::vector<double>>>()));
    std::vector<std::string> propOptions;
    propOptions.push_back("Multiply");
    propOptions.push_back("Divide");
    declareProperty("Operation","Multiply",boost::make_shared<StringListValidator>(propOptions),
    "The operation with which the correction is applied to the data (default: Multiply)");
  }
  
  void PolynomialCorrection::retrieveProperties()
  {
    // So this will be m_coeffs[0] + m_coeffs[1]*x + m_coeffs[2]*x^2 + ...
    m_coeffs = getProperty("Coefficients");
    m_polySize = m_coeffs.size();
    std::string operation = getProperty("Operation");
    m_isOperationMultiply = operation == "Multiply"?true:false;
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
    
    // Apply the  correction factor to the data and error
    if (m_isOperationMultiply)
    {
      YOut = YIn*factor;
      EOut = EIn*std::abs(factor);
    }
    else 
    {
      YOut = YIn/factor;
      EOut = EIn/std::abs(factor);
    }
  }

} // namespace Algorithms
} // namespace Mantid
