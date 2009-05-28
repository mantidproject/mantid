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

using namespace Kernel;

// Get a reference to the logger
Logger& Lorentzian1D::g_log = Logger::get("Lorentzian1D");


void Lorentzian1D::declareParameters()
{
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  declareProperty("bg0",0.0, Direction::InOut);
  declareProperty("bg1",0.0, Direction::InOut);
  declareProperty("height",0.0, Direction::InOut);
  declareProperty("peakCentre",0.0, Direction::InOut);
  declareProperty("hwhm",1.0, positiveDouble, "", Direction::InOut);
}



void Lorentzian1D::function(double* in, double* out, double* xValues, double* yValues, double* yErrors, int nData)
{
    double bg0 = in[0];
    double bg1 = in[1];
    double height = in[2];
    double peakCentre = in[3];
    double hwhm = in[4];

    for (size_t i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        double Yi = height*( hwhm*hwhm/(diff*diff+hwhm*hwhm) )+bg0+bg1*xValues[i];
        out[i] = (Yi - yValues[i])/yErrors[i];
    }
}

void Lorentzian1D::functionDeriv(double* in, double* out, double* xValues, double* yValues, double* yErrors, int nData)
{
    double height = in[2];
    double peakCentre = in[3];
    double hwhm = in[4];

    int nParam = m_parameterNames.size();
    for (size_t i = 0; i < nData; i++) {
        double s = yErrors[i];
        double diff = xValues[i]-peakCentre;
        double e = ( hwhm*hwhm/(diff*diff+hwhm*hwhm) )/s;
        double invDenominator =  1/(diff*diff+hwhm*hwhm);
        out[i*nParam + 0] = 1/s;
        out[i*nParam + 1] = xValues[i]/s;
        out[i*nParam + 2] = hwhm*hwhm*invDenominator/s;
        out[i*nParam + 3] = -2.0*diff*hwhm*hwhm*invDenominator*invDenominator/s;
        out[i*nParam + 4] = (-hwhm*hwhm*invDenominator+1)*2.0*hwhm*invDenominator /s;
    }
}

} // namespace CurveFitting
} // namespace Mantid
