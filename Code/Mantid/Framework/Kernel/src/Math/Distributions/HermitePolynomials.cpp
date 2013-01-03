//--------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------
#include "MantidKernel/Math/Distributions/HermitePolynomials.h"

#include <algorithm>

namespace Mantid { namespace Kernel
{
  namespace Math
  {
    /**
     * @param n The order of polynomial (n>=0)
     * @param x Value of x to evaluate the polynomial
     */
    double hermitePoly(const unsigned int n, const double x)
    {
      if(n == 0) return 1.0;
      else if(n == 1) return 2*x;
      else
      {
        return 2.0*x*hermitePoly(n-1,x) - 2.0*(n-1)*hermitePoly(n-2,x) ;
      }
    }

    /**
     * @param n The order of polynomial (n>=0)
     * @param xaxis A list of xvalues for which to compute the nth polynomial
     * @returns A list of the values for each x value
     */
    std::vector<double> hermitePoly(const unsigned int n, const std::vector<double> & xaxis)
    {
      struct GeneratePoly
      {
        GeneratePoly(const unsigned int n, const std::vector<double> & xaxis) :
          npoly(n), xvalues(xaxis), currentIndex(0) {}
        double operator()() {return hermitePoly(npoly, xvalues[currentIndex++]);}

        const unsigned int npoly;
        const std::vector<double> & xvalues;
        size_t currentIndex;
      };

      std::vector<double> result(xaxis.size());
      std::generate(result.begin(), result.end(), GeneratePoly(n, xaxis));
      return result;
    }
  }
 } // namespace Kernel
} // namespace Mantid
