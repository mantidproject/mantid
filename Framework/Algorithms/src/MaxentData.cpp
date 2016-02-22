#include "MantidAlgorithms/MaxentData.h"
#include <boost/shared_array.hpp>
#include <gsl/gsl_fft_complex.h>

namespace Mantid {
namespace Algorithms {

MaxentData::MaxentData(MaxentEntropy_sptr entropy)
    : m_entropy(entropy), m_angle(-1.), m_chisq(-1.) {}

void MaxentData::load(const std::vector<double> &data,
                      const std::vector<double> &errors,
                      const std::vector<double> &image, double background,
                      bool isComplex) {

  if ((data.size() != errors.size()) || (image.size() % data.size())) {
    // If data and errors have N datapoints, image should have:
    // 2*X*N data points (complex data)
    // X*N data points (real data)
    throw std::runtime_error("Couldn't load invalid data");
  }
  if (m_background == 0) {
    throw std::runtime_error("Background must be positive");
  }

  m_angle = -1.;
  m_chisq = -1.;

  m_image = image;
  m_dataCalc = transformImageToData(image);
  m_background = background;

  if (isComplex) {
    m_data = data;
    m_errors = errors;
  } else {
    size_t size = data.size();

    m_data = std::vector<double>(2 * size);
    m_errors = std::vector<double>(2 * size);

    for (size_t i = 0; i < size; i++) {
      m_data[2 * i] = data[i];
      m_data[2 * i + 1] = 0.;
      ;
      m_errors[2 * i] = errors[i];
      m_errors[2 * i + 1] = 0.;
      ;
    }
  }
}

void MaxentData::correctImage() {

  for (auto &im : m_image) {
    im = m_entropy->correctValue(im);
  }

  // Reset m_angle and m_chisq to default
  m_angle = -1.;
  m_chisq = -1.;
}

void MaxentData::setImage(const std::vector<double> &image) {

  if (image.size() != m_image.size()) {
    throw std::invalid_argument("New image must be the same size");
  }
  m_image = image;

  // Reset m_angle and m_chisq to default
  m_angle = -1.;
  m_chisq = -1.;
}

std::vector<double> MaxentData::getChiGrad() const {

  if ((m_data.size() != m_errors.size()) ||
      (m_data.size() != m_dataCalc.size())) {
    throw std::invalid_argument("Cannot compute gradient of Chi");
  }

  size_t size = m_data.size();

  // Calculate gradient of Chi
  // CGrad_i = -2 * [ data_i - dataCalc_i ] / [ error_i ]^2
  std::vector<double> cgrad(size, 0.);
  for (size_t i = 0; i < size; i++) {
    if (m_errors[i])
      cgrad[i] = -2. * (m_data[i] - m_dataCalc[i]) / m_errors[i] / m_errors[i];
  }

  return cgrad;
}

std::vector<double> MaxentData::getEntropy() const {

  throw std::runtime_error("Not implemented");
}

std::vector<double> MaxentData::getEntropyGrad() const {

  std::vector<double> entropyGrad(m_image.size(), 0.);

  for (auto &im : m_image) {
    entropyGrad.push_back(m_entropy->getDerivative(im / m_background));
  }

  return entropyGrad;
}

std::vector<double> MaxentData::getMetric() const {

  std::vector<double> metric(m_image.size(), 0.);

  for (auto &im : m_image) {
    metric.push_back(m_entropy->getSecondDerivative(im));
  }

  return metric;
}

std::vector<std::vector<double>> MaxentData::getSearchDirections() {

  return m_directionsIm;
}
QuadraticCoefficients MaxentData::getQuadraticCoefficients() {

  return m_coeffs;
}

double MaxentData::getAngle() { return m_angle; }
double MaxentData::getChisq() { return m_chisq; }

void MaxentData::calculateSearchDirections() {

  // Two search directions
  const size_t dim = 2;

  // Some checks
  // TODO: check if they are needed
  if (m_dataCalc.size() != m_image.size()) {
    throw std::invalid_argument("Couldn't calculate the search directions");
  }

  size_t npoints = m_image.size();

  // Calculate data from start image
  m_dataCalc = transformImageToData(m_image);

  // Gradient of chi (in image space)
  std::vector<double> cgrad = transformDataToImage(getChiGrad());
  // Gradient of entropy
  std::vector<double> sgrad = getEntropyGrad();
  // Metric
  std::vector<double> metric = getMetric();

  double cnorm = 0.;
  double snorm = 0.;
  double csnorm = 0.;

  // Here we calculate:
  // SB. eq 22 -> |grad S|, |grad C|
  // SB. eq 37 -> test
  for (size_t i = 0; i < npoints; i++) {
    cnorm += cgrad[i] * cgrad[i] * metric[i] * metric[i];
    snorm += sgrad[i] * sgrad[i] * metric[i] * metric[i];
    csnorm += cgrad[i] * sgrad[i] * metric[i] * metric[i];
  }
  cnorm = sqrt(cnorm);
  snorm = sqrt(snorm);

  m_angle = sqrt(0.5 * (1. - csnorm / snorm / cnorm));
  // csnorm could be greater than snorm * cnorm due to rounding issues
  // so check for nan
  if (m_angle != m_angle)
    m_angle = 0.;

  // Calculate the search directions

  // Search directions (image space)
  m_directionsIm =
      std::vector<std::vector<double>>(2, std::vector<double>(npoints, 0.));

  for (size_t i = 0; i < npoints; i++) {
    m_directionsIm[0][i] = metric[i] * cgrad[i] / cnorm;
    m_directionsIm[1][i] = metric[i] * sgrad[i] / snorm;
    // xi1[i] = image[i] * (sgrad[i] / snorm - cgrad[i] / cnorm);
  }

  // Search directions (data space)
  // Not needed outside this method
  auto directionsDat =
      std::vector<std::vector<double>>(2, std::vector<double>(npoints, 0.));
  directionsDat[0] = transformImageToData(m_directionsIm[0]);
  directionsDat[1] = transformImageToData(m_directionsIm[1]);

  double chiSq = getChisq();
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
    // Note: the factor chiSQ has to go either here or in calculateChi
    m_coeffs.c1[k][0] /= chiSq;
  }

  // Then s2, c2
  m_coeffs.s2 = Kernel::DblMatrix(dim, dim);
  m_coeffs.c2 = Kernel::DblMatrix(dim, dim);
  for (size_t k = 0; k < dim; k++) {
    for (size_t l = 0; l < k + 1; l++) {
      m_coeffs.s2[k][l] = 0.;
      m_coeffs.c2[k][l] = 0.;
      for (size_t i = 0; i < npoints; i++) {
        m_coeffs.c2[k][l] += directionsDat[k][i] * directionsDat[l][i] /
                             m_errors[i] / m_errors[i];
        m_coeffs.s2[k][l] -=
            m_directionsIm[k][i] * m_directionsIm[l][i] / metric[i];
      }
      // Note: the factor chiSQ has to go either here or in calculateChi
      m_coeffs.c2[k][l] *= 2.0 / chiSq;
      m_coeffs.s2[k][l] *= 1.0 / m_background;
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

std::vector<double>
MaxentData::transformImageToData(const std::vector<double> &input) {

  /* Performs backward Fourier transform */

  size_t n = input.size();

  if (n % 2) {
    throw std::invalid_argument("Cannot transform to data space");
  }

  /* Prepare the data */
  boost::shared_array<double> result(new double[n]);
  for (size_t i = 0; i < n; i++) {
    result[i] = input[i];
  }

  /* Backward FT */
  gsl_fft_complex_wavetable *wavetable = gsl_fft_complex_wavetable_alloc(n / 2);
  gsl_fft_complex_workspace *workspace = gsl_fft_complex_workspace_alloc(n / 2);
  gsl_fft_complex_inverse(result.get(), 1, n / 2, wavetable, workspace);
  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);

  std::vector<double> output(n);
  for (size_t i = 0; i < n; i++) {
    output[i] = result[i];
  }

  return output;
}
std::vector<double>
MaxentData::transformDataToImage(const std::vector<double> &input) {

  /* Performs forward Fourier transform */

  size_t n = input.size();

  if (n % 2) {
    throw std::invalid_argument("Cannot transform to data space");
  }

  /* Prepare the data */
  boost::shared_array<double> result(new double[n]);
  for (size_t i = 0; i < n; i++) {
    result[i] = input[i];
  }

  /*  Fourier transofrm */
  gsl_fft_complex_wavetable *wavetable = gsl_fft_complex_wavetable_alloc(n / 2);
  gsl_fft_complex_workspace *workspace = gsl_fft_complex_workspace_alloc(n / 2);
  gsl_fft_complex_forward(result.get(), 1, n / 2, wavetable, workspace);
  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);

  /* Get the data */
  std::vector<double> output(n);
  for (size_t i = 0; i < n; i++) {
    output[i] = result[i];
  }

  return output;
}

} // namespace Algorithms
} // namespace Mantid
