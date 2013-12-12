/*WIKI*
A Lorentzian function is defined as: 

<center><math> \frac{A}{\pi} \left( \frac{\frac{\Gamma}{2}}{(x-x_0)^2 + (\frac{\Gamma}{2})^2}\right)</math></center>

where:
    <UL>
    <LI> A (Amplitude) - Maximum peak height at peak centre </LI>
    <LI><math>x_0</math> (PeakCentre) - centre of peak </LI>
    <LI><math>\Gamma</math> (HWHM) - half-width at half-maximum </LI>
    </UL>

Note that the FWHM (Full Width Half Maximum) equals two times HWHM, and the integral over the Lorentzian equals 1.

The figure below illustrate this symmetric peakshape function fitted to a TOF peak:

[[Image:LorentzianWithConstBackground.png]]
 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Lorentzian.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Lorentzian);

void Lorentzian::init()
{
  declareParameter("Amplitude", 1.0, "Maximum height of peak when x=x0");
  declareParameter("PeakCentre", 0.0, "Centre of peak");
  declareParameter("FWHM", 0.0, "Falf-width at half-maximum");
}


void Lorentzian::functionLocal(double* out, const double* xValues, const size_t nData)const
{
    const double amplitude = getParameter("Amplitude");
    const double peakCentre = getParameter("PeakCentre");
    const double halfGamma = 0.5*getParameter("FWHM");

    const double invPI = 1.0/M_PI;
    for (size_t i = 0; i < nData; i++)
    {
        double diff=(xValues[i]-peakCentre);
        out[i] = amplitude*invPI*halfGamma/(diff*diff + (halfGamma*halfGamma));
    }
}

void Lorentzian::functionDerivLocal(Jacobian* out, const double* xValues, const size_t nData)
{
    const double amplitude = getParameter("Amplitude");
    const double peakCentre = getParameter("PeakCentre");
    const double gamma = getParameter("FWHM");
    const double halfGamma = 0.5*gamma;

    const double invPI = 1.0/M_PI;
    for (size_t i = 0; i < nData; i++)
    {
        double diff = xValues[i]-peakCentre;
        const double invDen1 = 1.0/(gamma*gamma + 4.0*diff*diff);
        const double dfda = 2.0*invPI*gamma*invDen1;
        out->set(i,0, dfda);

        double invDen2 =  1/(diff*diff + halfGamma*halfGamma);
        const double dfdxo = amplitude*invPI*gamma*diff*invDen2*invDen2;
        out->set(i,1, dfdxo);

        const double dfdg = -2.0*amplitude*invPI*(gamma*gamma - 4.0*diff*diff)*invDen1*invDen1;
        out->set(i,2, dfdg);
    }

}


} // namespace CurveFitting
} // namespace Mantid
