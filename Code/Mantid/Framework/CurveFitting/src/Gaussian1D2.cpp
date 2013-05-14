/*WIKI*
Instead of using this algorithm to fit a Gaussian, 
please use the [[Fit]] algorithm where the Function parameter of this algorithm is used to specified the fitting function, including selecting a [[Gaussian]].
*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Gaussian1D2.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace CurveFitting
{

/// Empty default constructor
Gaussian1D2::Gaussian1D2()
{
  useAlgorithm("Fit");
  deprecatedDate("2011-08-16");
}

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Gaussian1D2)

/// Sets documentation strings for this algorithm
void Gaussian1D2::initDocs()
{
  this->setWikiSummary("== Deprecation notice == Instead of using this algorithm to fit a Gaussian, please use the [[Fit]] algorithm where the Function parameter of this algorithm is used to specified the fitting function, including selecting a [[Gaussian]]. ");
  this->setOptionalMessage("== Deprecation notice == Instead of using this algorithm to fit a Gaussian, please use the Fit algorithm where the Function parameter of this algorithm is used to specified the fitting function, including selecting a Gaussian.");
}


using namespace Kernel;

void Gaussian1D2::declareParameters()
{
  declareProperty("BG0", 0.0, "Constant background value (default 0)", Direction::InOut);
  declareProperty("BG1", 0.0, "Linear background modelling parameter (default 0)", Direction::InOut);
  declareProperty("Height", 0.0, "Height of peak (default 0)", Direction::InOut);
  declareProperty("PeakCentre", 0.0, "Centre of peak (default 0)", Direction::InOut);

  auto positiveDouble = boost::make_shared<BoundedValidator<double> >();
  positiveDouble->setLower(std::numeric_limits<double>::min());

  declareProperty("Sigma", 1.0, positiveDouble, "Standard deviation (default 1)", Direction::InOut);
}

void Gaussian1D2::modifyStartOfRange(double& startX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  startX = peak_val-(6*sigma);
}

void Gaussian1D2::modifyEndOfRange(double& endX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  endX = peak_val+(6*sigma);
}

void Gaussian1D2::modifyInitialFittedParameters(std::vector<double>& fittedParameter)
{
  const double sigma = getProperty("Sigma");

  fittedParameter[4] = 1/(sigma*sigma);  // the fitting is actually done on 1/sigma^2, also referred to as the weight
}

void Gaussian1D2::modifyFinalFittedParameters(std::vector<double>& fittedParameter) 
{
  double weight = fittedParameter[4];

  fittedParameter[4] = sqrt(1/weight); // to convert back to sigma
}

void Gaussian1D2::function(const double* in, double* out, const double* xValues, const size_t nData)
{
    const double bg0 = in[0];
    const double bg1 = in[1];
    const double height = in[2];
    const double peakCentre = in[3];
    const double weight = in[4];

    for (size_t i = 0; i < nData; i++) 
    {
        double diff=xValues[i]-peakCentre;
        out[i] = height*exp(-0.5*diff*diff*weight)+bg0+bg1*xValues[i];
    }
}

void Gaussian1D2::functionDeriv(const double* in, Jacobian* out, const double* xValues, const size_t nData)
{
    const double height = in[2];
    const double peakCentre = in[3];
    const double weight = in[4];

    for (int i = 0; i < static_cast<int>(nData); i++) {
        double diff = xValues[i]-peakCentre;
        double e = exp(-0.5*diff*diff*weight);
        out->set(i,0, 1);
        out->set(i,1, xValues[i]);
        out->set(i,2, e);
        out->set(i,3, diff*height*e*weight);
        out->set(i,4, -0.5*diff*diff*height*e);
    }
}

} // namespace CurveFitting
} // namespace Mantid
