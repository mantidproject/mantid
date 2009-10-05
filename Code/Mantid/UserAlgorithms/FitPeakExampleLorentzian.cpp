#include "FitPeakExampleLorentzian.h"

namespace Mantid
{
namespace CurveFitting
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FitPeakExampleLorentzian)

using namespace Mantid::Kernel;
using namespace Mantid::API;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

// Fitting parameters are defined here
void FitPeakExampleLorentzian::declareParameters()
{
  // A standard fitting parameter is added by specifying its "name", default value and 
  // "description" (the last argument "Direction::InOut" you will rarely need to change, but
  // is required to add)
  declareProperty("BG0", 0.0, "Constant background value (default 0)", Direction::InOut);
  declareProperty("BG1", 0.0, "Linear background modelling parameter (default 0)",Direction::InOut);
  declareProperty("Height", 0.0, "Peak height (may be refined to a negative value to fit a dipped curve)", Direction::InOut);
  declareProperty("PeakCentre",0.0,  "Centre of peak (default 0)", Direction::InOut);

  // Some parameters are not allowed to take certain values. E.g. the full width of half maximum (FWHM) is
  // not allowed to be negative and zero (the zero may be discussed, but for the here assume this)
  // Then first create an BoundedValidator instance as shown below
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(std::numeric_limits<double>::min()); // set here the lowest value to the lowest 
                                                                // possible number the CPU can store

  // Finally to use this BoundedValidator add it as the 3rd argument
  declareProperty("HWHM",1.0, positiveDouble, "half-width at half-maximum (default 1)", Direction::InOut);
}

// Fitting function is added here. The arguments contains the following:
// in:      Input fitting parameter values, store in the order in which they are defined in declareParameters()
// out:     peak shape function values
// xValues: X values for data points
// nData:   Number of data points
void FitPeakExampleLorentzian::function(const double* in, double* out, const double* xValues, const int& nData)
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


} // namespace CurveFitting
} // namespace Mantid