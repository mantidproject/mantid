/*WIKI*
A Lorentzian function is defined as:

:<math> \mbox{Height}* \left( \frac{\mbox{HWHM}^2}{(x-\mbox{PeakCentre})^2+\mbox{HWHM}^2} \right) </math>

where

    <UL>
    <LI> Height - height of peak (at maximum) </LI>
    <LI> PeakCentre - centre of peak </LI>
    <LI> HWHM - half-width at half-maximum </LI>
    </UL>

Note that the FWHM (Full Width Half Maximum) equals two times HWHM, and the integral over the Lorentzian equals <math>\mbox{Height} * \pi * \mbox{HWHM}</math> (ignoring the linear background). In the literature you may also often see the notation <math>\gamma</math> = HWHM.

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

DECLARE_FUNCTION(Lorentzian)

void Lorentzian::init()
{
  declareParameter("Height", 0.0, "height of peak (not the height may be refined to a negative value to fit a dipped curve)");
  declareParameter("PeakCentre", 0.0, "Centre of peak");
  declareParameter("HWHM", 0.0, "half-width at half-maximum");
}


void Lorentzian::functionLocal(double* out, const double* xValues, const size_t nData)const
{
    const double height = getParameter("Height");
    const double peakCentre = getParameter("PeakCentre");
    const double hwhm = getParameter("HWHM");

    for (size_t i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        out[i] = height*( hwhm*hwhm/(diff*diff+hwhm*hwhm) );
    }
}

void Lorentzian::functionDerivLocal(Jacobian* out, const double* xValues, const size_t nData)
{
    const double height = getParameter("Height");
    const double peakCentre = getParameter("PeakCentre");
    const double hwhm = getParameter("HWHM");

    for (size_t i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double invDenominator =  1/((diff*diff+hwhm*hwhm));
        out->set(i,0, hwhm*hwhm*invDenominator);
        out->set(i,1, 2.0*height*diff*hwhm*hwhm*invDenominator*invDenominator);
        out->set(i,2, height*(-hwhm*hwhm*invDenominator+1)*2.0*hwhm*invDenominator);
    }

}


} // namespace CurveFitting
} // namespace Mantid
