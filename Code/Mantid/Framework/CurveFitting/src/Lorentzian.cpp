//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Lorentzian.h"
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
  declareParameter("Height", 0.0);
  declareParameter("PeakCentre", 0.0);
  declareParameter("HWHM", 0.0);
}


void Lorentzian::functionLocal(double* out, const double* xValues, const size_t nData)const
{
    const double& height = getParameter("Height");
    const double& peakCentre = getParameter("PeakCentre");
    const double& hwhm = getParameter("HWHM");

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        out[i] = height*( hwhm*hwhm/(diff*diff+hwhm*hwhm) );
    }
}

void Lorentzian::functionDerivLocal(Jacobian* out, const double* xValues, const size_t nData)
{
    const double& height = getParameter("Height");
    const double& peakCentre = getParameter("PeakCentre");
    const double& hwhm = getParameter("HWHM");

    for (int i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double invDenominator =  1/((diff*diff+hwhm*hwhm));
        out->set(i,0, hwhm*hwhm*invDenominator);
        out->set(i,1, 2.0*height*diff*hwhm*hwhm*invDenominator*invDenominator);
        out->set(i,2, height*(-hwhm*hwhm*invDenominator+1)*2.0*hwhm*invDenominator);
    }

}


} // namespace CurveFitting
} // namespace Mantid
