// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------------------
#include "MantidCurveFitting/Functions/MultivariateGaussianComptonProfile.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
DECLARE_FUNCTION(MultivariateGaussianComptonProfile)

const char *MultivariateGaussianComptonProfile::AMP_PARAM = "Intensity";
const char *MultivariateGaussianComptonProfile::SIGMA_X_PARAM = "SigmaX";
const char *MultivariateGaussianComptonProfile::SIGMA_Y_PARAM = "SigmaY";
const char *MultivariateGaussianComptonProfile::SIGMA_Z_PARAM = "SigmaZ";
const char *MultivariateGaussianComptonProfile::STEPS_ATTR = "IntegrationSteps";

MultivariateGaussianComptonProfile::MultivariateGaussianComptonProfile()
    : ComptonProfile(), m_integrationSteps(256), m_thetaStep(0.0), m_phiStep(0.0) {}

/**
 * @returns A string containing the name of the function
 */
std::string MultivariateGaussianComptonProfile::name() const { return "MultivariateGaussianComptonProfile"; }

void MultivariateGaussianComptonProfile::declareParameters() {
  ComptonProfile::declareParameters();
  declareParameter(AMP_PARAM, 1.0, "Gaussian intensity parameter");
  declareParameter(SIGMA_X_PARAM, 1.0, "Sigma X parameter");
  declareParameter(SIGMA_Y_PARAM, 1.0, "Sigma Y parameter");
  declareParameter(SIGMA_Z_PARAM, 1.0, "Sigma Z parameter");
}

void MultivariateGaussianComptonProfile::declareAttributes() {
  ComptonProfile::declareAttributes();
  declareAttribute(STEPS_ATTR, IFunction::Attribute(m_integrationSteps));
}

/**
 * @param name The name of the attribute
 * @param value The attribute's value
 */
void MultivariateGaussianComptonProfile::setAttribute(const std::string &name, const Attribute &value) {
  ComptonProfile::setAttribute(name, value);
  if (name == STEPS_ATTR) {
    int steps = value.asInt();

    if (steps < 1)
      throw std::runtime_error(std::string(STEPS_ATTR) + " attribute must be positive and non-zero");

    if (steps % 2 == 1)
      throw std::runtime_error(std::string(STEPS_ATTR) + " attribute must be an even number");

    m_integrationSteps = steps;
    m_thetaStep = M_PI / steps;
    m_phiStep = (M_PI / 2.0) / steps;
  }
}

std::vector<size_t> MultivariateGaussianComptonProfile::intensityParameterIndices() const {
  return std::vector<size_t>(1, this->parameterIndex(AMP_PARAM));
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
size_t MultivariateGaussianComptonProfile::fillConstraintMatrix(Kernel::DblMatrix &cmatrix, const size_t start,
                                                                const HistogramData::HistogramE &errors) const {
  std::vector<double> result(ySpace().size());
  this->massProfile(result.data(), ySpace().size());
  std::transform(result.begin(), result.end(), errors.begin(), result.begin(), std::divides<double>());
  cmatrix.setColumn(start, result);
  return 1;
}

/**
 * @param result A pre-sized output array that should be filled with the
 *               results
 * @param nData The size of the array
 */
void MultivariateGaussianComptonProfile::massProfile(double *result, const size_t nData) const {
  const double amplitude(getParameter(AMP_PARAM));
  this->massProfile(result, nData, amplitude);
}

void MultivariateGaussianComptonProfile::massProfile(double *result, const size_t nData, const double amplitude) const {
  std::vector<double> s2Cache;
  buildS2Cache(s2Cache);

  const double sigmaX(getParameter(SIGMA_X_PARAM));
  const double sigmaY(getParameter(SIGMA_Y_PARAM));
  const double sigmaZ(getParameter(SIGMA_Z_PARAM));

  const double prefactorJ = (1.0 / (sqrt(2.0 * M_PI) * sigmaX * sigmaY * sigmaZ)) * (2.0 / M_PI);
  const double prefactorFSE =
      (pow(sigmaX, 4) + pow(sigmaY, 4) + pow(sigmaZ, 4)) / (9.0 * sqrt(2.0 * M_PI) * sigmaX * sigmaY * sigmaZ);

  const auto &yspace = ySpace();
  const auto &modq = modQ();
  if (modq.empty() || yspace.empty()) {
    throw std::runtime_error("Y values or Q values not set");
  }

  for (size_t i = 0; i < nData; i++) {
    const double y(yspace[i]);
    const double q(modq[i]);

    double j = prefactorJ * calculateJ(s2Cache, y);
    double fse = (prefactorFSE / q) * calculateFSE(s2Cache, y);

    result[i] = amplitude * (j + fse);
  }
}

/**
 * @brief Calculates the mass profile
 * @param s2Cache Cache of S2 values
 * @param y Y value
 * @return Mass profile
 */
double MultivariateGaussianComptonProfile::calculateJ(const std::vector<double> &s2Cache, double y) const {
  double sum(0.0);

  for (int i = 0; i < m_integrationSteps; i++) {
    for (int j = 0; j < m_integrationSteps; j++) {
      double s2 = s2Cache[i * m_integrationSteps + j];
      sum += intervalCoeff(i, j) * calculateIntegrandJ(s2, y);
    }
  }

  double fact = (m_thetaStep * m_phiStep) / 9.0;

  return fact * sum;
}

/**
 * @brief Calculates the A3 FSE correction.
 * @param s2Cache Cache of S2 values
 * @param y Y value
 * @return Additive FSE correction
 */
double MultivariateGaussianComptonProfile::calculateFSE(const std::vector<double> &s2Cache, double y) const {
  double sum(0.0);

  for (int i = 0; i < m_integrationSteps; i++) {
    for (int j = 0; j < m_integrationSteps; j++) {
      double s2 = s2Cache[i * m_integrationSteps + j];
      sum += intervalCoeff(i, j) * calculateIntegrandFSE(s2, y);
    }
  }

  double fact = (m_thetaStep * m_phiStep) / 9.0;

  return fact * sum;
}

/**
 * @brief Obtains a cell of the coefficient grid for Simpson's integration in
 *        2D.
 * @param i X index
 * @param j Y index
 * @return Coefficient
 *
 * [ 1  4 2  4 1 ]
 * [ 4 16 8 16 4 ]
 * [ 2  8 4  8 2 ]
 * [ 4 16 8 16 4 ]
 * [ 1  4 2  4 1 ]
 */
double MultivariateGaussianComptonProfile::intervalCoeff(int i, int j) const {
  double a = 1.0;
  double b = 1.0;

  if (i > 0 && i <= m_integrationSteps)
    a = i % 2 == 1 ? 4 : 2;

  if (j > 0 && j <= m_integrationSteps)
    b = j % 2 == 1 ? 4 : 2;

  return a * b;
}

/**
 * @brief Caches values of S2 for all theta and phi in integration range.
 * @param s2Cache Reference to vector to cache S2 values in
 */
void MultivariateGaussianComptonProfile::buildS2Cache(std::vector<double> &s2Cache) const {
  s2Cache.clear();

  double sigmaX2(getParameter(SIGMA_X_PARAM));
  double sigmaY2(getParameter(SIGMA_Y_PARAM));
  double sigmaZ2(getParameter(SIGMA_Z_PARAM));

  sigmaX2 *= sigmaX2;
  sigmaY2 *= sigmaY2;
  sigmaZ2 *= sigmaZ2;

  for (int i = 0; i <= m_integrationSteps; i++) {
    const double theta = m_thetaStep * i;
    for (int j = 0; j <= m_integrationSteps; j++) {
      const double phi = m_phiStep * j;

      double sinTheta2 = pow(sin(theta), 2);
      double sinPhi2 = pow(sin(phi), 2);
      double cosTheta2 = pow(cos(theta), 2);
      double cosPhi2 = pow(cos(phi), 2);

      double x = (sinTheta2 * cosPhi2) / sigmaX2;
      double y = (sinTheta2 * sinPhi2) / sigmaY2;
      double z = cosTheta2 / sigmaZ2;

      double s2 = x + y + z;

      s2 = 1.0 / s2;

      s2Cache.emplace_back(s2);
    }
  }
}

} // namespace Mantid::CurveFitting::Functions
