//------------------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------------------
#include "MantidCurveFitting/Functions/MultivariateGaussianComptonProfile.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
DECLARE_FUNCTION(MultivariateGaussianComptonProfile)

/**
 */
MultivariateGaussianComptonProfile::MultivariateGaussianComptonProfile()
    : ComptonProfile() {}

/**
 * @returns A string containing the name of the function
 */
std::string MultivariateGaussianComptonProfile::name() const {
  return "MultivariateGaussianComptonProfile";
}

/**
 */
void MultivariateGaussianComptonProfile::declareParameters() {
  // TODO
}

/*
 */
std::vector<size_t>
MultivariateGaussianComptonProfile::intensityParameterIndices() const {
  // TODO
}

/**
 * Fills in a column of the matrix with this mass profile, starting at the given
 * index
 * @param cmatrix InOut matrix whose column should be set to the mass profile
 *                for each active hermite polynomial
 * @param start Index of the column to start on
 * @param errors The data errors
 * @returns The number of columns filled
 */
size_t MultivariateGaussianComptonProfile::fillConstraintMatrix(
    Kernel::DblMatrix &cmatrix, const size_t start,
    const std::vector<double> &errors) const {
  // TODO
  return 0;
}

/**
 * @param result A pre-sized output array that should be filled with the
 *               results
 * @param nData The size of the array
 */
void MultivariateGaussianComptonProfile::massProfile(double *result,
                                                     const size_t nData) const {
  const double amplitude(getParameter(1));
  this->massProfile(result, nData, amplitude);
}

/**
 * @param result A pre-sized output vector that should be filled with the
 *               results
 * @param nData The size of the array
 * @param amplitude A fixed value for the amplitude
 */
void MultivariateGaussianComptonProfile::massProfile(
    double *result, const size_t nData, const double amplitude) const {
  // TODO
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
