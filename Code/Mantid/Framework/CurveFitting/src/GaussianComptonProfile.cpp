//------------------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------------------
#include "MantidCurveFitting/GaussianComptonProfile.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid
{
  namespace CurveFitting
  {
    DECLARE_FUNCTION(GaussianComptonProfile);

    const char * WIDTH_PARAM = "Width";
    const char * AMP_PARAM = "Intensity";

    const double STDDEV_TO_HWHM = std::sqrt(std::log(4.0));

    /**
     */
    GaussianComptonProfile::GaussianComptonProfile()
      : ComptonProfile()
    {
    }

    /**
     * @returns A string containing the name of the function
     */
    std::string GaussianComptonProfile::name() const
    {
      return "GaussianComptonProfile";
    }

    /**
     */
    void GaussianComptonProfile::declareParameters()
    {
      // DO NOT REORDER WITHOUT CHANGING THE GETPARAMETER calls
      //
      declareParameter(WIDTH_PARAM, 1.0, "Gaussian width parameter");
      declareParameter(AMP_PARAM, 1.0, "Gaussian intensity parameter");
    }

    /**
     * Uses a Gaussian approximation for the mass and convolutes it with the Voigt
     * instrument resolution function
     * @param result An pre-sized output vector that should be filled with the results
     */
    void GaussianComptonProfile::massProfile(std::vector<double> & result) const
    {
      double lorentzPos(0.0), gaussWidth(getParameter(0)), amplitude(getParameter(1));
      double gaussFWHM = std::sqrt(std::pow(resolutionFWHM(),2) + std::pow(2.0*STDDEV_TO_HWHM*gaussWidth,2));

      const auto & yspace = ySpace();

      // Gaussian already folded into Voigt
      voigtApprox(result, yspace, lorentzPos, amplitude, lorentzFWHM(), gaussFWHM);
      std::vector<double> voigtDiffResult(yspace.size());
      voigtApproxDiff(voigtDiffResult, yspace, lorentzPos, amplitude, lorentzFWHM(), gaussFWHM);

      const auto & modq = modQ();
      const size_t nData(result.size());
      for(size_t j = 0; j < nData; ++j)
      {
        const double factor = std::pow(gaussWidth,4.0)/(3.0*modq[j]);
        result[j] -= factor*voigtDiffResult[j];
      }
    }


  } // namespace CurveFitting
} // namespace Mantid
