//------------------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------------------
#include "MantidCurveFitting/GaussianComptonProfile.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {
DECLARE_FUNCTION(GaussianComptonProfile);

const char *WIDTH_PARAM = "Width";
const char *AMP_PARAM = "Intensity";

const double STDDEV_TO_HWHM = std::sqrt(std::log(4.0));

/**
 */
GaussianComptonProfile::GaussianComptonProfile() : ComptonProfile() {}

/**
 * @returns A string containing the name of the function
 */
std::string GaussianComptonProfile::name() const {
  return "GaussianComptonProfile";
}

/**
 */
void GaussianComptonProfile::declareParameters() {
  // DO NOT REORDER WITHOUT CHANGING THE getParameter AND
  // intensityParameterIndices methods
  //
  declareParameter(WIDTH_PARAM, 1.0, "Gaussian width parameter");
  declareParameter(AMP_PARAM, 1.0, "Gaussian intensity parameter");
}

/*
 */
std::vector<size_t> GaussianComptonProfile::intensityParameterIndices() const {
  return std::vector<size_t>(1, this->parameterIndex(AMP_PARAM));
}

/**
 * Fills in a column of the matrix with this mass profile, starting at the given
 * index
 * @param cmatrix InOut matrix whose column should be set to the mass profile
 * for each active hermite polynomial
 * @param start Index of the column to start on
 * @param errors The data errors
 * @returns The number of columns filled
 */
size_t GaussianComptonProfile::fillConstraintMatrix(
    Kernel::DblMatrix &cmatrix, const size_t start,
    const std::vector<double> &errors) const {
  std::vector<double> result(ySpace().size());
  const double amplitude = 1.0;
  this->massProfile(result.data(), ySpace().size(), amplitude);
  std::transform(result.begin(), result.end(), errors.begin(), result.begin(),
                 std::divides<double>());
  cmatrix.setColumn(start, result);
  return 1;
}

/**
 * Uses a Gaussian approximation for the mass and convolutes it with the Voigt
 * instrument resolution function
 * @param result An pre-sized output array that should be filled with the
 * results
 * @param nData The size of the array
 */
void GaussianComptonProfile::massProfile(double *result,
                                         const size_t nData) const {
  const double amplitude(getParameter(1));
  this->massProfile(result, nData, amplitude);
}

/**
 * Uses a Gaussian approximation for the mass and convolutes it with the Voigt
 * instrument resolution function
 * @param result An pre-sized output vector that should be filled with the
 * results
 * @param nData The size of the array
 * @param amplitude A fixed value for the amplitude
 */
void GaussianComptonProfile::massProfile(double *result, const size_t nData,
                                         const double amplitude) const {
  double lorentzPos(0.0), gaussWidth(getParameter(0));
  double gaussFWHM =
      std::sqrt(std::pow(m_resolutionFunction->resolutionFWHM(), 2) +
                std::pow(2.0 * STDDEV_TO_HWHM * gaussWidth, 2));

  const auto &yspace = ySpace();
  // Gaussian already folded into Voigt
  std::vector<double> voigt(yspace.size()), voigtDiffResult(yspace.size());
  m_resolutionFunction->voigtApprox(voigt, yspace, lorentzPos, amplitude,
                                    m_resolutionFunction->lorentzFWHM(),
                                    gaussFWHM);
  voigtApproxDiff(voigtDiffResult, yspace, lorentzPos, amplitude,
                  m_resolutionFunction->lorentzFWHM(), gaussFWHM);

  const auto &modq = modQ();
  const auto &ei = e0();
  // Include e_i^0.1*mass/q pre-factor
  for (size_t j = 0; j < nData; ++j) {
    const double q = modq[j];
    const double prefactor = mass() * std::pow(ei[j], 0.1) / q;
    result[j] =
        prefactor *
        (voigt[j] - std::pow(gaussWidth, 4.0) * voigtDiffResult[j] / (3.0 * q));
  }
}

} // namespace CurveFitting
} // namespace Mantid
