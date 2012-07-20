#include "MantidCurveFitting/HighOrderPolynomialBackground.h"
#include "MantidAPI/FunctionFactory.h"


using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace CurveFitting
{

DECLARE_FUNCTION(HighOrderPolynomialBackground)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  HighOrderPolynomialBackground::HighOrderPolynomialBackground()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  HighOrderPolynomialBackground::~HighOrderPolynomialBackground()
  {
  }
  
  void HighOrderPolynomialBackground::init()
  {
    declareParameter("A0", 0.0);
    declareParameter("A1", 0.0);
    declareParameter("A2", 0.0);
    declareParameter("A3", 0.0);
    declareParameter("A4", 0.0);
  }


  /*
   *
   */
  void HighOrderPolynomialBackground::function1D(double* out, const double* xValues, const size_t nData)const
  {
      const double& a0 = getParameter("A0");
      const double& a1 = getParameter("A1");
      const double& a2 = getParameter("A2");
      const double& a3 = getParameter("A3");
      const double& a4 = getParameter("A4");

      for (size_t i = 0; i < nData; i++)
      {
          out[i] = a0+a1*xValues[i]+a2*xValues[i]*xValues[i]+a3*xValues[i]*xValues[i]*xValues[i]+
                  a4*xValues[i]*xValues[i]*xValues[i]*xValues[i];
      }

      return;
  }

  void HighOrderPolynomialBackground::functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
  {
      for (size_t i = 0; i < nData; i++)
      {
          out->set(i, 0, 1);
          out->set(i, 1, xValues[i]);
          out->set(i, 2, xValues[i]*xValues[i]);
          out->set(i, 3, xValues[i]*xValues[i]*xValues[i]);
          out->set(i, 4, xValues[i]*xValues[i]*xValues[i]*xValues[i]);
      }
  }



} // namespace CurveFitting
} // namespace Mantid
