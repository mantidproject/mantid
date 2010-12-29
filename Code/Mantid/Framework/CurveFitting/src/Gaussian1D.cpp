//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Gaussian1D.h"
//#include "MantidAlgorithms/MaskBins.h"

//#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Gaussian1D)

using namespace Kernel;
using namespace API;

void Gaussian1D::declareParameters()
{
  declareProperty("BG0", 0.0, "Constant background value (default 0)", Direction::InOut);
  declareProperty("Height", 0.0, "Height of peak (default 0)", Direction::InOut);
  declareProperty("PeakCentre",0.0, "Centre of peak (default 0)", Direction::InOut);

  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(std::numeric_limits<double>::min());

  declareProperty("Sigma", 1.0, positiveDouble,
    "Standard deviation (default 1)", Direction::InOut);
}

void Gaussian1D::modifyStartOfRange(double& startX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  startX = peak_val-(6*sigma);
}

void Gaussian1D::modifyEndOfRange(double& endX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  endX = peak_val+(6*sigma);
}

void Gaussian1D::modifyInitialFittedParameters(std::vector<double>& fittedParameter)
{
  const double sigma = getProperty("Sigma");

  fittedParameter[3] = 1/(sigma*sigma);  // the fitting is actually done on 1/sigma^2, also referred to as the weight
}

void Gaussian1D::modifyFinalFittedParameters(std::vector<double>& fittedParameter) 
{
  double weight = fittedParameter[3];

  fittedParameter[3] = sqrt(1/weight); // to convert back to sigma
}

void Gaussian1D::function(const double* in, double* out, const double* xValues, const int& nData)
{
    const double& bg0 = in[0];
    const double& height = in[1];
    const double& peakCentre = in[2];
    const double& weight = in[3];

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        out[i] = height*exp(-0.5*diff*diff*weight)+bg0;
    }
}

void Gaussian1D::functionDeriv(const double* in, Jacobian* out, const double* xValues, const int& nData)
{
    const double& height = in[1];
    const double& peakCentre = in[2];
    const double& weight = in[3];

    for (int i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double e = exp(-0.5*diff*diff*weight);
        out->set(i,0, 1);
        out->set(i,1, e);
        out->set(i,2, diff*height*e*weight);
        out->set(i,3, -0.5*diff*diff*height*e);
    }
}

} // namespace CurveFitting
} // namespace Mantid
