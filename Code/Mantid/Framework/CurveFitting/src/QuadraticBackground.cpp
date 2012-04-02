#include "MantidCurveFitting/QuadraticBackground.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace CurveFitting
{

DECLARE_FUNCTION(QuadraticBackground)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  QuadraticBackground::QuadraticBackground()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  QuadraticBackground::~QuadraticBackground()
  {
    // TODO Auto-generated destructor stub
  }
  
  void QuadraticBackground::init()
  {
    declareParameter("A0", 0.0);
    declareParameter("A1", 0.0);
    declareParameter("A2", 0.0);
  }


  /*
   *
   */
  void QuadraticBackground::function1D(double* out, const double* xValues, const size_t nData)const
  {
      const double& a0 = getParameter("A0");
      const double& a1 = getParameter("A1");
      const double& a2 = getParameter("A2");

      for (size_t i = 0; i < nData; i++) {
          out[i] = a0+a1*xValues[i]+a2*xValues[i]*xValues[i];
      }
  }

  void QuadraticBackground::functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
  {
      for (size_t i = 0; i < nData; i++) {
          out->set(i, 0, 1);
          out->set(i, 1, xValues[i]);
          out->set(i, 2, xValues[i]*xValues[i]);
      }
  }


} // namespace Mantid
} // namespace CurveFitting

