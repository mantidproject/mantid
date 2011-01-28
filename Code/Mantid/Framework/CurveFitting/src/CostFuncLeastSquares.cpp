//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncLeastSquares.h"

namespace Mantid
{
namespace CurveFitting
{

DECLARE_COSTFUNCTION(CostFuncLeastSquares,Least squares)

  /// Calculate value of cost function from observed and calculated values
  /// note yCal modified for computational efficiency
  /// @param yData :: Array of yData
  /// @param inverseError :: Array of inverse error values
  /// @param yCal :: Calculated y
  /// @param n :: The number of points 
  /// @return The calculated cost value
  double CostFuncLeastSquares::val(const double* yData, const double* inverseError, double* yCal, const int& n)
  {
    for (int i = 0; i < n; i++)
      yCal[i] = (  yCal[i] - yData[i] ) * inverseError[i];

    double retVal = 0.0;

    for (int i = 0; i < n; i++)
      retVal += yCal[i]*yCal[i];

    return retVal;
  }

  /// Calculate the derivatives of the cost function
  void CostFuncLeastSquares::deriv(const double* yData, const double* inverseError, const double* yCal, 
                     const double* jacobian, double* outDerivs, const int& p, const int& n)
  {
    for (int iP = 0; iP < p; iP++) 
    {
      outDerivs[iP] = 0.0;
      for (int iY = 0; iY < n; iY++) 
      {
        outDerivs[iP] += 2.0*(yCal[iY]-yData[iY]) * jacobian[iY*p + iP] 
                        * inverseError[iY]*inverseError[iY];
      }
    }
  }



} // namespace CurveFitting
} // namespace Mantid
