//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Lorentzian1D.h"
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_multifit_nlin.h>

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Lorentzian1D)

/// Sets documentation strings for this algorithm
void Lorentzian1D::initDocs()
{
  this->setWikiSummary("== Deprecation notice == Instead of using this algorithm to fit a Lorentzian please use the [[Fit]] algorithm where the Function parameter of this algorithm is used to specified the fitting function, including selecting a [[Lorentzian]]. ");
  this->setOptionalMessage("== Deprecation notice == Instead of using this algorithm to fit a Lorentzian please use the Fit algorithm where the Function parameter of this algorithm is used to specified the fitting function, including selecting a Lorentzian.");
}


using namespace Kernel;

void Lorentzian1D::declareParameters()
{
  declareProperty("BG0", 0.0, "Constant background value (default 0)", Direction::InOut);
  declareProperty("BG1", 0.0, "Linear background modelling parameter (default 0)",Direction::InOut);
  declareProperty("Height", 0.0, "Peak height (may be refined to a negative value to fit a dipped curve)", Direction::InOut);
  declareProperty("PeakCentre",0.0,  "Centre of peak (default 0)", Direction::InOut);

  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(std::numeric_limits<double>::min());

  declareProperty("HWHM",1.0, positiveDouble, "half-width at half-maximum (default 1)", Direction::InOut);
}

void Lorentzian1D::function(const double* in, double* out, const double* xValues, const size_t nData)
{
    const double& bg0 = in[0];
    const double& bg1 = in[1];
    const double& height = in[2];
    const double& peakCentre = in[3];
    const double& hwhm = in[4];

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        out[i] = height*( hwhm*hwhm/(diff*diff+hwhm*hwhm) )+bg0+bg1*xValues[i];
    }
}

void Lorentzian1D::functionDeriv(const double* in, Jacobian* out, const double* xValues, const size_t nData)
{
    const double& height = in[2];
    const double& peakCentre = in[3];
    const double& hwhm = in[4];

    for (int i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double invDenominator =  1/((diff*diff+hwhm*hwhm));
        out->set(i,0, 1);
        out->set(i,1, xValues[i]);
        out->set(i,2, hwhm*hwhm*invDenominator);
        out->set(i,3, 2.0*height*diff*hwhm*hwhm*invDenominator*invDenominator);
        out->set(i,4, height*(-hwhm*hwhm*invDenominator+1)*2.0*hwhm*invDenominator);
    }
}

} // namespace CurveFitting
} // namespace Mantid
