#include "FitPeakExampleLorentzianUseDerivatives.h"

namespace Mantid
{
namespace CurveFitting
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FitPeakExampleLorentzianUseDerivatives)

using namespace Mantid::Kernel;
using namespace Mantid::API;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

// Fitting parameters are defined here
void FitPeakExampleLorentzianUseDerivatives::declareParameters()
{
  // A standard fitting parameter is added by specifying its "name", default value and 
  // "description" (the last argument "Direction::InOut" you will rarely need to change)
  declareProperty("BG0", 0.0, "Constant background value (default 0)", Direction::InOut);
  declareProperty("BG1", 0.0, "Linear background modelling parameter (default 0)",Direction::InOut);
  declareProperty("Height", 0.0, "Peak height (may be refined to a negative value to fit a dipped curve)", Direction::InOut);
  declareProperty("PeakCentre",0.0,  "Centre of peak (default 0)", Direction::InOut);

  // Some parameters are not allowed to take certain values. E.g. the full width of half maximum (FWHM) is
  // not allowed to be negative and zero.
  // First create an BoundedValidator instance:
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(std::numeric_limits<double>::min()); // set here the lowest value to the lowest 
                                                                // possible number the CPU can store

  // Secondly insert this BoundedValidator as the 3rd argument
  declareProperty("HWHM",1.0, positiveDouble, "half-width at half-maximum (default 1)", Direction::InOut);
}

// Fitting function is added here. The arguments store the following:
// in:      Input fitting parameter values, stored in the order in which they are defined in declareParameters()
// out:     peak shape function values at all data points 
// xValues: X values for data points
// nData:   Number of data points
void FitPeakExampleLorentzianUseDerivatives::function(const double* in, double* out, const double* xValues, const int& nData)
{
    const double& bg0 = in[0];          // first parameter defined in declareParameters()
    const double& bg1 = in[1];          // second parameter defined in declareParameters()
    const double& height = in[2];       // etc...
    const double& peakCentre = in[3];
    const double& hwhm = in[4];

    // Finally simply calculate the function here
    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        out[i] = height*( hwhm*hwhm/(diff*diff+hwhm*hwhm) )+bg0+bg1*xValues[i];
    }
}


// Specify derivates of function. The arguments store the following:
// in:      Input fitting parameter values, stored in the order in which they are defined in declareParameters()
// out:     the derivate at each point with respect to each parameter 
// xValues: X values for data points
// nData:   Number of data points
void FitPeakExampleLorentzianUseDerivatives::functionDeriv(const double* in, Jacobian* out, const double* xValues, const int& nData)
{
    // No need to get hold of first two parameters specified in declareParameters(), i.e.
    // the background parameters BG0 and BG1 since they appear as separate terms in the expression for 
    // this function and to 1st order. See description in FitPeakExampleLorentzianUseDerivatives.h
    const double& height = in[2];     // Third parameter defined in declareParameters()
    const double& peakCentre = in[3]; // Forth parameter defined in declareParameters()
    const double& hwhm = in[4];       // etc....

    // here calculate derivatives
    for (int i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double invDenominator =  1/((diff*diff+hwhm*hwhm));
        out->set(i,0, 1);                               // with respect to first param defined in declareParameters()
        out->set(i,1, xValues[i]);                      // with respect to second param defined in declareParameters()
        out->set(i,2, hwhm*hwhm*invDenominator);        // etc....
        out->set(i,3, 2.0*height*diff*hwhm*hwhm*invDenominator*invDenominator);
        out->set(i,4, height*(-hwhm*hwhm*invDenominator+1)*2.0*hwhm*invDenominator);
    }
}

} // namespace CurveFitting
} // namespace Mantid