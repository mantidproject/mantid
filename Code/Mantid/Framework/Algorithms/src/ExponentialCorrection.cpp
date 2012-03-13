/*WIKI* 


This algorithm corrects the data and error values on a workspace by the value of an exponential function
of the form <math> {\rm C0} e^{-{\rm C1} x} </math>.
This formula is calculated for each data point, with the value of ''x'' 
being the mid-point of the bin in the case of histogram data.
The data and error values are either divided or multiplied by the value of this function, according to the
setting of the Operation property.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ExponentialCorrection.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{
  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(ExponentialCorrection)
  
  /// Sets documentation strings for this algorithm
  void ExponentialCorrection::initDocs()
  {
    this->setWikiSummary("Corrects the data in a workspace by the value of an exponential function which is evaluated at the X value of each data point. ");
    this->setOptionalMessage("Corrects the data in a workspace by the value of an exponential function which is evaluated at the X value of each data point.");
  }
  

  void ExponentialCorrection::defineProperties()
  {
    declareProperty("C0",1.0);
    declareProperty("C1",0.0);
    
    std::vector<std::string> operations(2);
    operations[0] = "Multiply";
    operations[1] = "Divide";
    declareProperty("Operation", "Divide", boost::make_shared<Kernel::StringListValidator>(operations));
  }
  
  void ExponentialCorrection::retrieveProperties()
  {
    m_c0 = getProperty("C0");
    m_c1 = getProperty("C1");
    std::string op = getProperty("Operation");
    m_divide = ( op == "Divide" ) ? true : false;
  }
  
  void ExponentialCorrection::performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut)
  {
    double factor = m_c0 * exp(-1.0*m_c1*XIn);
    if (m_divide) factor = 1.0/factor;
    
    // Multiply the data and error by the correction factor
    YOut = YIn*factor;
    EOut = EIn*factor;
  }

} // namespace Algorithms
} // namespace Mantid
