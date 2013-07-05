/*WIKI*
This function calculates a partial CubicSpline expansion

:<math> \sum_{n=0}^N a_n T_n(a+bx) </math>

where <math> a_n </math> are the expansion coefficients and <math> T_n(x) </math> are
CubicSpline polynomials of the first kind defined by the reccurence relation

:<math>T_0(x)=1 \,\!</math>

:<math>T_1(x)=x \,\!</math>

:<math>T_{n+1}(x)= 2xT_n(x)-T_{n-1}(x) \,\!</math>

Coefficients <math> a </math> and <math> b </math> are defined to map the fitting interval
into [-1,1] interval.

CubicSpline function has tree attributes (non-fitting parameters). First is 'n' which has
integer type and sets the expansion order and creates n+1 expansion coefficients (fitting
parameters). The parameter names have the form 'Ai' where 'A' is letter 'A' and 'i' is the
parameter's index starting from 0.

The other two attributes are doubles 'StartX' and 'EndX' which define the expansion (fitting) interval.

 *WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CubicSpline.h"
#include "MantidAPI/FunctionFactory.h"

#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <vector>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(CubicSpline)

CubicSpline::CubicSpline()
{
  //Create interpolation accelerator and spline
  acc = gsl_interp_accel_alloc ();
  spline = gsl_spline_alloc (gsl_interp_cspline, 2);

  //number of interpolation points, default of two
  declareAttribute("n",Attribute(2));

  //default value of the two default interpolation attributes
  declareAttribute("x0", Attribute(0.0));
  declareAttribute("x1", Attribute(1.0));

  //default value of corresponding y parameters
  declareParameter("y0", 0);
  declareParameter("y1", 0);
};

void CubicSpline::function1D(double* out, const double* xValues, const size_t nData)const
{
  int n = getAttribute("n").asInt();

  //Create interpolation accelerator and spline object


  //Create vectors for x and y
  std::vector<double> x(n);
  std::vector<double> y(n);

  for(int i = 0; i < n; ++i) {
     std::string num = boost::lexical_cast<std::string>(i);
     std::string xName = "x" + num;
     std::string yName = "y" + num;

     x[i] = getAttribute(xName).asDouble();
     y[i] = getParameter(yName);
   }

  gsl_spline_init (spline, x.data(), y.data(), n);

  for (size_t i = 0; i < nData; ++i)
  {
    out[i] = gsl_spline_eval (spline, xValues[i], acc);
  }


}


/**
 * @param attName :: The attribute name. If it is not "n" exception is thrown.
 * @param att :: An int attribute containing the new value. The value cannot be negative.
 */
void CubicSpline::setAttribute(const std::string& attName,const API::IFunction::Attribute& att)
{
  if(attName == "n") {
    int n = att.asInt();
    int oldN = getAttribute("n").asInt();

    if(n > oldN) {
      std::string oldXName = "x" + boost::lexical_cast<std::string>(oldN);
      double oldX = getAttribute(oldXName).asDouble();

      for(int i = oldN; i < n; ++i) {
        std::string num = boost::lexical_cast<std::string>(i);
        std::string newXName = "x" + num;
        std::string newYName = "y" + num;

        declareAttribute(newXName, Attribute(oldX + static_cast<double>(i-oldN+1)));
        declareParameter(newYName, 0);
      }
    } else {
      throw std::invalid_argument("Cubic Spline: Can't decrease the number of attributes");
    }
  }

  storeAttributeValue( attName, att );
}

CubicSpline::~CubicSpline() {
  //destroy handles to the spline and accelerator
  gsl_spline_free (spline);
  gsl_interp_accel_free (acc);
}

} // namespace CurveFitting
} // namespace Mantid
