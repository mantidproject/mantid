// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MaxEnt/MaxentCalculator.h"
#include <cmath>

namespace Mantid {
namespace Algorithms {

/**
 * Constructor
 * @param entropy : pointer to MaxentEntropy object defining the entropy formula
 * to use
 * @param transform : pointer to MaxentTransform object defining how to
 * transform from data space to image space and vice-versa
 */
MaxentCalculator::MaxentCalculator(MaxentEntropy_sptr entropy,
                                   MaxentTransform_sptr transform)
    : m_data(), m_errors(), m_image(), m_dataCalc(), m_background(1.0),
      m_angle(-1.), m_chisq(-1.), m_directionsIm(), m_coeffs(),
      m_entropy(entropy), m_transform(transform) {}

/**
 * Calculates the gradient of chi-square using the experimental data, calculated
 * data and errors
 * @return : The gradient of chi-square as a vector
 */
std::vector<double> MaxentCalculator::calculateChiGrad() const {

  // Calculates the gradient of Chi
  // CGrad_i = -2 * [ data_i - dataCalc_i ] / [ error_i ]^2

  if ((m_data.size() != m_errors.size()) ||
      (m_dataCalc.size() % m_data.size())) {
    // Data and errors must have the same number of data points
    // but the reconstructed (calculated) data may contain more points
    throw std::runtime_error("Cannot compute gradient of Chi");
  }

  // We only consider the experimental data points to calculate chi grad
  size_t sizeDat = m_data.size();
  // The number of calculated data points can be bigger than the number of
  // experimental data points I am not sure how we should deal with this
  // situation. On the one hand one can only consider real data and errors to
  // calculate chi-square, but on the other hand this method should return a
  // vector of size equal to the size of the calculated data, so I am just
  // setting the 'leftovers' to zero. This is what is done in the original
  // muon code.
  size_t sizeDatCalc = m_dataCalc.size();
  std::vector<double> cgrad(sizeDatCalc, 0.);
  auto dpoints = static_cast<double>(sizeDat);
  for (size_t i = 0; i < sizeDat; i++) {
    if (m_errors[i] != 0)
      cgrad[i] = -2. * (m_data[i] - m_dataCalc[i]) / m_errors[i] / m_errors[i] /
                 dpoints;
  }

  return cgrad;
}

/**
 * Returns the reconstructed (calculated) data
 * @return : The reconstructed data as a vector
 */
std::vector<double> MaxentCalculator::getReconstructedData() const {

  if (m_dataCalc.empty()) {
    // If it is empty it means we didn't load valid data
    throw std::runtime_error("No data were loaded");
  }
  return m_dataCalc;
}

/**
 * Returns the (reconstructed) image
 * @return : The image as a vector
 */
std::vector<double> MaxentCalculator::getImage() const {

  if (m_image.empty()) {
    // If it is empty it means we didn't load valid data
    throw std::runtime_error("No image was loaded");
  }
  return m_image;
}

/**
 * Returns the search directions (in image space)
 * @return : The search directions
 */
std::vector<std::vector<double>> MaxentCalculator::getSearchDirections() const {

  if (m_directionsIm.empty()) {
    throw std::runtime_error("Search directions have not been calculated");
  }
  return m_directionsIm;
}

/**
 * Returns the quadratic coefficients
 * @return : The quadratic coefficients
 */
QuadraticCoefficients MaxentCalculator::getQuadraticCoefficients() const {

  if (!m_coeffs.c1.size().first) {
    // This means that none of the coefficients were calculated
    throw std::runtime_error("Quadratic coefficients have not been calculated");
  }
  return m_coeffs;
}

/**
 * Returns the angle between the gradient of chi-square and the gradient of the
 * entropy (calculated and initialized in calculateQuadraticCoefficients())
 * @return : The angle
 */
double MaxentCalculator::getAngle() const {

  if (m_angle == -1) {
    throw std::runtime_error("Angle has not been calculated");
  }
  return m_angle;
}

/**
 * Returns chi-square
 * @return : Chi-square
 */
double MaxentCalculator::getChisq() {

  if (m_chisq == -1.) {
    throw std::runtime_error("Chisq has not been calculated");
  }
  return m_chisq;
}

std::vector<double>
MaxentCalculator::calculateData(const std::vector<double> &image) const {
  return m_transform->imageToData(image);
}

std::vector<double>
MaxentCalculator::calculateImage(const std::vector<double> &data) const {
  return m_transform->dataToImage(data);
}

/**
 * Performs an iteration and calculates everything: search directions (SB. 21),
 * quadratic coefficients (SB. 22), angle between the gradient of chi-square and
 * the gradient of the entropy, and chi-sqr
 * @param data : [input] The experimental data as a vector (real or complex)
 * @param errors : [input] The experimental errors as a vector (real or complex)
 * @param image : [input] The image as a vector (real or complex)
 * @param background : [input] The background
 * @param linearAdjustments: [input] Optional linear adjustments (complex)
 * @param constAdjustments: [input] Optional constant adjustments (complex)
 */
void MaxentCalculator::iterate(const std::vector<double> &data,
                               const std::vector<double> &errors,
                               const std::vector<double> &image,
                               double background,
                               const std::vector<double> &linearAdjustments,
                               const std::vector<double> &constAdjustments) {
  // Some checks
  if (data.empty() || errors.empty() || (data.size() != errors.size())) {
    throw std::invalid_argument(
        "Cannot calculate quadratic coefficients: invalid data");
  }
  if (image.empty()) {
    throw std::invalid_argument(
        "Cannot calculate quadratic coefficients: invalid image");
  }
  if (background == 0) {
    throw std::invalid_argument(
        "Cannot calculate quadratic coefficients: invalid background");
  }
  m_data = data;
  m_errors = errors;
  m_image = m_entropy->correctValues(image, background);
  m_background = background;
  m_dataCalc = m_transform->imageToData(image);

  // Set to -1, these will be calculated later
  m_angle = -1.;
  m_chisq = -1.;

  // adjust calculated data, if required
  if (!linearAdjustments.empty()) {
    if (linearAdjustments.size() < m_dataCalc.size()) {
      throw std::invalid_argument(
          "Cannot adjust calculated data: too few linear adjustments");
    }
    for (size_t j = 0; j < m_dataCalc.size() / 2; ++j) {
      double yr = m_dataCalc[2 * j];
      double yi = m_dataCalc[2 * j + 1];
      m_dataCalc[2 * j] =
          yr * linearAdjustments[2 * j] - yi * linearAdjustments[2 * j + 1];
      m_dataCalc[2 * j + 1] =
          yi * linearAdjustments[2 * j] + yr * linearAdjustments[2 * j + 1];
    }
  }
  if (!constAdjustments.empty()) {
    if (constAdjustments.size() < m_dataCalc.size()) {
      throw std::invalid_argument(
          "Cannot adjust calculated data: too few constant adjustments");
    }
    for (size_t i = 0; i < m_dataCalc.size(); ++i) {
      m_dataCalc[i] += constAdjustments[i];
    }
  }

  // Two search directions
  const size_t dim = 2;

  size_t npoints = m_image.size();

  // Gradient of chi (in image space)
  std::vector<double> cgrad = m_transform->dataToImage(calculateChiGrad());
  // Gradient of entropy
  std::vector<double> sgrad = m_entropy->derivative(m_image, m_background);
  // Metric (second derivative of the entropy)
  std::vector<double> metric =
      m_entropy->secondDerivative(m_image, m_background);

  if (cgrad.size() != npoints || sgrad.size() != npoints ||
      metric.size() != npoints)
    throw std::runtime_error(
        "Cannot calculate quadratic coefficients: invalid image space");

  double cnorm = 0.;
  double snorm = 0.;
  double csnorm = 0.;

  // Here we calculate:
  // SB. eq 22 -> |grad S|, |grad C|
  // SB. eq 37 -> test
  for (size_t i = 0; i < npoints; i++) {
    auto metric2 = metric[i] * metric[i];
    cnorm += cgrad[i] * cgrad[i] * metric2;
    snorm += sgrad[i] * sgrad[i] * metric2;
    csnorm += cgrad[i] * sgrad[i] * metric2;
  }
  cnorm = sqrt(cnorm);
  snorm = sqrt(snorm);

  if (cnorm == 0) {
    cnorm = 1.;
  }
  if (snorm == 0) {
    snorm = 1.;
  }

  m_angle = sqrt(0.5 * (1. - csnorm / snorm / cnorm));
  // csnorm could be greater than snorm * cnorm due to rounding issues
  // so check for nan
  if (!std::isfinite(m_angle)) {
    m_angle = 0.;
  }

  // Calculate the search directions

  // Search directions (image space)
  m_directionsIm =
      std::vector<std::vector<double>>(2, std::vector<double>(npoints, 0.));

  for (size_t i = 0; i < npoints; i++) {
    m_directionsIm[0][i] = metric[i] * cgrad[i] / cnorm;
    m_directionsIm[1][i] = metric[i] * sgrad[i] / snorm;
  }

  // Search directions (data space)
  // Not needed outside this method
  auto directionsDat =
      std::vector<std::vector<double>>(2, std::vector<double>(npoints, 0.));
  directionsDat[0] = m_transform->imageToData(m_directionsIm[0]);
  directionsDat[1] = m_transform->imageToData(m_directionsIm[1]);

  calculateChisq();
  double factor = getChisq() * double(npoints) / 2;
  auto resolutionFactor =
      static_cast<double>(m_dataCalc.size() / m_data.size());

  // Calculate the quadratic coefficients SB. eq 24

  // First compute s1, c1
  m_coeffs.s1 = Kernel::DblMatrix(dim, 1);
  m_coeffs.c1 = Kernel::DblMatrix(dim, 1);
  for (size_t k = 0; k < dim; k++) {
    m_coeffs.c1[k][0] = m_coeffs.s1[k][0] = 0.;
    for (size_t i = 0; i < npoints; i++) {
      m_coeffs.s1[k][0] += m_directionsIm[k][i] * sgrad[i];
      m_coeffs.c1[k][0] += m_directionsIm[k][i] * cgrad[i];
    }
    m_coeffs.c1[k][0] /= factor;
  }

  // Then s2
  m_coeffs.s2 = Kernel::DblMatrix(dim, dim);
  for (size_t k = 0; k < dim; k++) {
    for (size_t l = 0; l < k + 1; l++) {
      m_coeffs.s2[k][l] = 0.;
      for (size_t i = 0; i < npoints; i++) {
        m_coeffs.s2[k][l] -=
            m_directionsIm[k][i] * m_directionsIm[l][i] / metric[i];
      }
    }
  }
  // Then c2
  npoints = m_errors.size();
  m_coeffs.c2 = Kernel::DblMatrix(dim, dim);
  for (size_t k = 0; k < dim; k++) {
    for (size_t l = 0; l < k + 1; l++) {
      m_coeffs.c2[k][l] = 0.;
      for (size_t i = 0; i < npoints; i++) {
        if (m_errors[i] != 0)
          m_coeffs.c2[k][l] += directionsDat[k][i] * directionsDat[l][i] /
                               m_errors[i] / m_errors[i];
      }
      m_coeffs.c2[k][l] *= 2.0 / factor * resolutionFactor;
    }
  }

  // Symmetrise s2, c2: reflect accross the diagonal
  for (size_t k = 0; k < dim; k++) {
    for (size_t l = k + 1; l < dim; l++) {
      m_coeffs.s2[k][l] = m_coeffs.s2[l][k];
      m_coeffs.c2[k][l] = m_coeffs.c2[l][k];
    }
  }
}

/**
 * Calculates chi-square
 */
void MaxentCalculator::calculateChisq() {
  if (m_data.empty() || m_errors.empty() || m_dataCalc.empty()) {
    throw std::runtime_error("Cannot calculate chi-square");
  }
  m_chisq = calculateChiSquared(m_dataCalc);
}

/// Calculate
/// ChiSq = 1 / N sum_i [ data_i - dataCalc_i ]^2 / [ error_i ]^2
double
MaxentCalculator::calculateChiSquared(const std::vector<double> &data) const {
  size_t npoints = m_data.size();
  auto dpoints = static_cast<double>(npoints);

  double chisq = 0;
  for (size_t i = 0; i < npoints; i++) {
    double term = (m_data[i] - data[i]) / m_errors[i];
    chisq += term * term;
  }
  return chisq / dpoints;
}

} // namespace Algorithms
} // namespace Mantid
