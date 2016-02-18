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
    : ComptonProfile(), m_integrationSteps(35) {}

/**
 * @returns A string containing the name of the function
 */
std::string MultivariateGaussianComptonProfile::name() const {
  return "MultivariateGaussianComptonProfile";
}

/**
 */
void MultivariateGaussianComptonProfile::declareParameters() {
  declareParameter(AMP_PARAM, 1.0, "Gaussian intensity parameter");
  declareParameter(SIGMA_X_PARAM, 1.0, "Sigma X parameter");
  declareParameter(SIGMA_Y_PARAM, 1.0, "Sigma Y parameter");
  declareParameter(SIGMA_Z_PARAM, 1.0, "Sigma Z parameter");
}

/**
 */
void MultivariateGaussianComptonProfile::declareAttributes() {
  ComptonProfile::declareAttributes();
  declareAttribute(STEPS_ATTR, IFunction::Attribute(m_integrationSteps));
}

/**
 * @param name The name of the attribute
 * @param value The attribute's value
 */
void MultivariateGaussianComptonProfile::setAttribute(const std::string &name,
                                                      const Attribute &value) {
  ComptonProfile::setAttribute(name, value);
  if (name == STEPS_ATTR) {
    int steps = value.asInt();
    if (steps < 1)
      throw std::runtime_error(std::string(STEPS_ATTR) +
                               " attribute must be positive and non-zero");

    m_integrationSteps = steps;
    m_thetaStep = 1.0 / steps;
    m_phiStep = (M_PI / 2.0) / steps;
  }
}

/*
 */
std::vector<size_t>
MultivariateGaussianComptonProfile::intensityParameterIndices() const {
  return std::vector<size_t>(0, this->parameterIndex(AMP_PARAM));
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
  std::vector<double> result(ySpace().size());
  this->massProfile(result.data(), ySpace().size());
  std::transform(result.begin(), result.end(), errors.begin(), result.begin(),
                 std::divides<double>());
  cmatrix.setColumn(start, result);
  return 1;
}

/**
 * @param result A pre-sized output array that should be filled with the
 *               results
 * @param nData The size of the array
 */
void MultivariateGaussianComptonProfile::massProfile(double *result,
                                                     const size_t nData) const {
  const double amplitude(getParameter(0));
  this->massProfile(result, nData, amplitude);
}

void MultivariateGaussianComptonProfile::massProfile(
    double *result, const size_t nData, const double amplitude) const {
  std::vector<double> s2Cache;
  buildS2Cache(s2Cache);

  const double sigmaX(getParameter(1));
  const double sigmaY(getParameter(2));
  const double sigmaZ(getParameter(3));

  const double prefactor =
      (1.0 / (sqrt(2.0 * M_PI) * sigmaX * sigmaY * sigmaZ)) * (2.0 / M_PI);
  const auto &yspace = ySpace();

  for (size_t i = 0; i < nData; i++) {
    double y = yspace[i];

    double sum = 0.0;

    for (int j = 0; j < m_integrationSteps; j++) {
      int thetaIdx = (m_integrationSteps + 1) * j;
      sum += (integratePhi(thetaIdx, s2Cache, y) +
              integratePhi(thetaIdx + (m_integrationSteps + 1), s2Cache, y)) *
             0.5;
    }

    sum *= m_thetaStep;

    result[i] = amplitude * prefactor * sum;
  }
}

double MultivariateGaussianComptonProfile::integratePhi(
    int idx, std::vector<double> &s2Cache, double y) const {
  double sum = 0.0;

  for (int i = 0; i < m_integrationSteps; i++) {
    sum += (calculateIntegrand(idx + i, s2Cache, y) +
            calculateIntegrand(idx + i + 1, s2Cache, y)) *
           0.5;
  }

  sum *= m_phiStep;

  return sum;
}

double MultivariateGaussianComptonProfile::calculateIntegrand(
    int idx, std::vector<double> &s2Cache, double y) const {
  double s2 = s2Cache[idx];
  double i = s2 * exp(-(y * y) / (2 * s2));
  return i;
}

/**
 * @brief Caches values of S2 for all theta and phi in integration range.
 * @param s2Cache Reference to vector to cache S2 values in
 */
void MultivariateGaussianComptonProfile::buildS2Cache(
    std::vector<double> &s2Cache) const {
  s2Cache.clear();

  double sigmaX2(getParameter(1));
  double sigmaY2(getParameter(2));
  double sigmaZ2(getParameter(3));

  sigmaX2 *= sigmaX2;
  sigmaY2 *= sigmaY2;
  sigmaZ2 *= sigmaZ2;

  const double num = sigmaX2 * sigmaY2 * sigmaZ2;

  for (int i = 0; i <= m_integrationSteps; i++) {
    const double theta = m_thetaStep * i;
    for (int j = 0; j <= m_integrationSteps; j++) {
      const double phi = m_phiStep * j;

      double x = sin(theta) * cos(phi);
      double y = sin(theta) * sin(phi);
      double z = cos(theta);

      double denom = (x * sigmaY2 * sigmaZ2) + (sigmaX2 * y * sigmaZ2) +
                     (sigmaX2 * sigmaY2 * z);

      double s2 = 0.0;
      if (denom != 0.0)
        s2 = num / denom;

      s2Cache.push_back(s2);
    }
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
