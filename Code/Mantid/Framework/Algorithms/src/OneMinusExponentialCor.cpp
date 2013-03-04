/*WIKI* 


This algorithm corrects the data and error values on a workspace by the value of one minus an exponential function
of the form <math> \rm C1(1 - e^{-{\rm C} x}) </math>.
This formula is calculated for each data point, with the value of ''x'' 
being the mid-point of the bin in the case of histogram data.
The data and error values are either divided or multiplied by the value of this function, according to the
setting of the Operation property.

This algorithm is now event aware.

This correction is applied to a copy of the input workpace and put into output workspace.
If the input and output workspaces have the same name, the operation is applied to the workspace of that name.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/OneMinusExponentialCor.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

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
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    declareProperty("C",1.0,mustBePositive,"The positive value by which the entire exponent calculation is multiplied (see formula below).");
    
    declareProperty("C1",1.0,"The value by which the entire calculation is multiplied (see formula below).");

    std::vector<std::string> operations(2);
    operations[0] = "Multiply";
    operations[1] = "Divide";
    declareProperty("Operation", "Divide", boost::make_shared<Kernel::StringListValidator>(operations),
      "Whether to divide (the default) or multiply the data by the correction function.");
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
