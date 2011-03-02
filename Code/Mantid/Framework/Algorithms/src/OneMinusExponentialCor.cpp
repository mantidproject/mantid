//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/OneMinusExponentialCor.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{
  // Register the class into the algorithm factory
  DECLARE_ALGORITHM(OneMinusExponentialCor)
  
  /// Sets documentation strings for this algorithm
  void OneMinusExponentialCor::initDocs()
  {
    this->setWikiSummary("Corrects the data in a workspace by one minus the value of an exponential function. ");
    this->setOptionalMessage("Corrects the data in a workspace by one minus the value of an exponential function.");
  }
  

  void OneMinusExponentialCor::defineProperties()
  {
    BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
    mustBePositive->setLower(0.0);
    declareProperty("C",1.0,mustBePositive);
    
    declareProperty("C1",1.0);

    std::vector<std::string> operations(2);
    operations[0] = "Multiply";
    operations[1] = "Divide";
    declareProperty("Operation", "Divide", new Kernel::ListValidator(operations)); 
  }
  
  void OneMinusExponentialCor::retrieveProperties()
  {
    m_c = getProperty("C");
    m_c1 = getProperty("C1");
    std::string op = getProperty("Operation");
    m_divide = ( op == "Divide" ) ? true : false;
  }
  
  void OneMinusExponentialCor::performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut)
  {
    double factor = m_c1*(1.0 - exp(-1.0*m_c*XIn));
    if (m_divide) factor = 1.0/factor;
    
    // Multiply the data and error by the correction factor
    YOut = YIn*factor;
    EOut = EIn*factor;
  }

} // namespace Algorithms
} // namespace Mantid
