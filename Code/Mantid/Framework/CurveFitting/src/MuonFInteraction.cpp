/*WIKI*
Muon F interaction function defined by
:<math>  \exp((-{Lambda} \times {x})^{Beta} ) \times  \frac {A} {6} \times
( 3 + B + C + D ) </math>
where
:<math> B = \cos( \sqrt 3 \times  Omega \times x) ) </math> ,
:<math> C = (1 - \frac{1}{\sqrt{3}} ) \times \cos ( ( ( 3 - \sqrt{3} ) / 2 ) \times Omega \times x ) </math>  and
:<math> D = (1 + \frac{1}{\sqrt{3}} ) \times \cos ( ( ( 3 + \sqrt{3} ) / 2 ) \times Omega \times x ) </math> .
 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/MuonFInteraction.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(MuonFInteraction)

void MuonFInteraction::init()
{
    declareParameter("Lambda", 0.2, "decay rate");
    declareParameter("Omega", 0.5, "angular frequency");
    declareParameter("Beta", 1, "exponent");
    declareParameter("A", 1, "Amplitude at 0");
}


void MuonFInteraction::function1D(double* out, const double* xValues, const size_t nData)const
{
  const double lambda = getParameter("Lambda");
  const double omega = getParameter("Omega");
  const double beta = getParameter("Beta");
  const double A = getParameter("A"); 
  const double sqrt3 = sqrt(3.0);

  for (size_t i = 0; i < nData; i++) {
    double A1=exp(-pow(lambda*xValues[i],beta))*A/6;
    double A2=cos(sqrt3*omega*xValues[i]);
    double A3=(1.0-1.0/sqrt3)*cos(((3.0-sqrt3)/2.0)*omega*xValues[i]);
    double A4=(1.0+1.0/sqrt3)*cos(((3.0+sqrt3)/2.0)*omega*xValues[i]);

    out[i] = A1*(3+A2+A3+A4);
  }
}

} // namespace CurveFitting
} // namespace Mantid
