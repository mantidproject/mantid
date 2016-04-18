#include "MantidAlgorithms/MaxEnt/MaxentData.h"
#include <boost/shared_array.hpp>
#include <gsl/gsl_fft_complex.h>

namespace Mantid {
namespace Algorithms {

/**
* Constructor
* @param entropy : pointer MaxentEntropy object defining the entropy formula to
* use
*/
MaxentData::MaxentData(MaxentEntropy_sptr entropy)
    : m_data(), m_errors(), m_image(), m_dataCalc(), m_background(1.0),
      m_angle(-1.), m_chisq(-1.), m_entropy(entropy), m_directionsIm(),
      m_coeffs() {}

/**
* Loads a real signal
* @param data : [input] A vector containing the experimental data
* @param errors : [input] A vector containing the experimental errors
* @param image : [input] A starting distribution for the image
* @param background : [input] A background level
* use
*/
void MaxentData::loadReal(const std::vector<double> &data,
                          const std::vector<double> &errors,
                          const std::vector<double> &image, double background) {

  if (data.size() != errors.size()) {
    // Data and errors must have the same number of points
    throw std::runtime_error("Couldn't load invalid data");
  }
  if (image.size() % (2 * data.size())) {
    // If data and errors have N datapoints, image should have 2*F*N datapoints
    // Where F is an integer factor
    throw std::runtime_error("Couldn't load invalid image");
  }
  if (background == 0) {
    throw std::runtime_error("Background must be positive");
  }

  size_t size = data.size();

  initImageSpace(image, background);

  m_data = std::vector<double>(2 * size);
  m_errors = std::vector<double>(2 * size);
  // Load the experimental (measured data)
  // Even indices correspond to the real part
  // Odd indices correspond to the imaginary part
  for (size_t i = 0; i < size; i++) {
    m_data[2 * i] = data[i];
    m_data[2 * i + 1] = 0.;
    m_errors[2 * i] = errors[i];
    m_errors[2 * i + 1] = 0.;
  }
}

/**
* Loads a complex signal
* @param dataRe : [input] A vector containing the real part of the experimental
* data
* @param dataIm : [input] A vector containing the imaginary part of the
* experimental data
* @param errorsRe : [input] A vector containing the experimental errors
* (associated with the real part)
* @param errorsIm : [input] A vector containing the experimental errors
* (associated with the imaginary part)
* @param image : [input] A starting distribution for the image
* @param background : [input] A background level
* use
*/
void MaxentData::loadComplex(const std::vector<double> &dataRe,
                             const std::vector<double> &dataIm,
                             const std::vector<double> &errorsRe,
                             const std::vector<double> &errorsIm,
                             const std::vector<double> &image,
                             double background) {

  if ((dataRe.size() != dataIm.size()) || (errorsRe.size() % errorsIm.size()) ||
      (dataRe.size() != errorsRe.size())) {
    // Real and imaginary components must have the same number of datapoints
    throw std::runtime_error("Couldn't load invalid data");
  }
  if (image.size() % (2 * dataRe.size())) {
    // If real and imaginary parts have N datapoints, image should have 2N
    // datapoints
    throw std::runtime_error("Couldn't load invalid image");
  }
  if (background == 0) {
    throw std::runtime_error("Background must be positive");
  }

  size_t size = dataRe.size();

  initImageSpace(image, background);

  m_data = std::vector<double>(2 * size);
  m_errors = std::vector<double>(2 * size);
  // Load the experimental (measured data)
  // Even indices correspond to the real part
  // Odd indices correspond to the imaginary part
  for (size_t i = 0; i < size; i++) {
    m_data[2 * i] = dataRe[i];
    m_data[2 * i + 1] = dataIm[i];
    m_errors[2 * i] = errorsRe[i];
    m_errors[2 * i + 1] = errorsIm[i];
  }
}

/**
* Initializes some of the member variables, those which are common to real and
* complex data
* @param image : [input] A starting distribution for the image
* @param background : [input] The background or sky level
*/
void MaxentData::initImageSpace(const std::vector<double> &image,
                                double background) {

  // Set to -1, these will be calculated later
  m_angle = -1.;
  m_chisq = -1.;
  // Load image, calculated data and background
  m_image = image;
  m_background = background;
  correctImage();
  m_dataCalc = transformImageToData(image);
}

/**
* Corrects the image according to the type of entropy
*/
void MaxentData::correctImage() {

  for (auto &im : m_image) {
    im = m_entropy->correctValue(im, m_background);
  }

  // Reset m_angle and m_chisq to default
  m_angle = -1.;
  m_chisq = -1.;
}

/**
* Updates the image according to an increment delta. Uses the previously
* calculated search directions.
* @param delta : [input] The increment to be added to the image
*/
void MaxentData::updateImage(const std::vector<double> &delta) {

  if (m_image.empty()) {
    throw std::runtime_error("No data were loaded");
  }
  if (m_directionsIm.empty()) {
    throw std::runtime_error("Search directions haven't been calculated");
  }
  if (delta.size() != m_directionsIm.size()) {
    throw std::invalid_argument("Image couldn't be updated");
  }
  // Calculate the new image
  for (size_t i = 0; i < m_image.size(); i++) {
    for (size_t k = 0; k < delta.size(); k++) {
      m_image[i] = m_image[i] + delta[k] * m_directionsIm[k][i];
    }
  }
  correctImage();

  m_dataCalc = transformImageToData(m_image);

  calculateChisq();
  m_chisq = getChisq();

  // Reset m_angle to default
  m_angle = -1.;
}

/**
* Calculates the gradient of chi-square using the experimental data, calculated
* data and errors
* @return : The gradient of chi-square as a vector
*/
std::vector<double> MaxentData::calculateChiGrad() const {

  // Calculates the gradient of Chi
  // CGrad_i = -2 * [ data_i - dataCalc_i ] / [ error_i ]^2

  if ((m_data.size() != m_errors.size()) ||
      (m_dataCalc.size() % m_data.size())) {
    // Data and errors must have the same number of data points
    // but the reconstructed (calculated) data may contain more points
    throw std::invalid_argument("Cannot compute gradient of Chi");
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
  for (size_t i = 0; i < sizeDat; i++) {
    if (m_errors[i] != 0)
      cgrad[i] = -2. * (m_data[i] - m_dataCalc[i]) / m_errors[i] / m_errors[i];
  }

  return cgrad;
}

/**
* Calculates the entropy (not needed for the moment)
* @return : The entropy as a vector
*/
std::vector<double> MaxentData::calculateEntropy() const {

  throw std::runtime_error("Not implemented");
}

/**
* Calculates the gradient of the entropy (depends on the type of entropy)
* @return : The gradient of the entropy as a vector
*/
std::vector<double> MaxentData::calculateEntropyGrad() const {

  const size_t size = m_image.size();

  std::vector<double> entropyGrad(size, 0.);

  for (size_t i = 0; i < size; i++) {
    entropyGrad[i] = m_entropy->getDerivative(m_image[i] / m_background);
  }

  return entropyGrad;
}

/**
* Returns the reconstructed (calculated) data
* @return : The reconstructed data as a vector
*/
std::vector<double> MaxentData::getReconstructedData() const {

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
std::vector<double> MaxentData::getImage() const {

  if (m_image.empty()) {
    // If it is empty it means we didn't load valid data
    throw std::runtime_error("No data were loaded");
  }
  return m_image;
}

/**
* Calculates the metric (depends on the type of entropy)
* @return : The metric as a vector
*/
std::vector<double> MaxentData::calculateMetric() const {

  const size_t size = m_image.size();

  std::vector<double> metric(size, 0.);

  for (size_t i = 0; i < size; i++) {
    metric[i] = m_entropy->getSecondDerivative(m_image[i]);
  }

  return metric;
}

/**
* Returns the search directions (in image space)
* @return : The search directions
*/
std::vector<std::vector<double>> MaxentData::getSearchDirections() const {

  return m_directionsIm;
}

/**
* Returns the quadratic coefficients
* @return : The quadratic coefficients
*/
QuadraticCoefficients MaxentData::getQuadraticCoefficients() const {

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
double MaxentData::getAngle() const {

  if (m_angle == -1) {
    throw std::runtime_error("Angle has not been calculated");
  }
  return m_angle;
}

/**
* Returns chi-square (it is calculated if necessary)
* @return : Chi-square
*/
double MaxentData::getChisq() {

  if (m_data.empty() || m_errors.empty() || m_dataCalc.empty()) {
    throw std::runtime_error("Cannot get chi-square");
  }
  // If data were loaded we can calculate chi-square
  if (m_chisq == -1.)
    calculateChisq();

  return m_chisq;
}

/**
* Calculates the search directions and quadratic coefficients (equations SB. 21
* and SB. 22). Also calculates the angle between the gradient of chi-square and
* the gradient of the entropy
*/
void MaxentData::calculateQuadraticCoefficients() {

  // Two search directions
  const size_t dim = 2;

  // Some checks
  if (m_data.empty() || m_errors.empty() || m_image.empty() ||
      m_dataCalc.empty()) {
    throw std::runtime_error("Data were not loaded");
  }
  if (m_dataCalc.size() != m_image.size()) {
    throw std::invalid_argument("Couldn't calculate the search directions");
  }

  size_t npoints = m_image.size();

  // Gradient of chi (in image space)
  std::vector<double> cgrad = transformDataToImage(calculateChiGrad());
  // Gradient of entropy
  std::vector<double> sgrad = calculateEntropyGrad();
  // Metric
  std::vector<double> metric = calculateMetric();

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

  if (cnorm == 0) {
    cnorm = 1.;
  }
  if (snorm == 0) {
    snorm = 1.;
  }

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
    // m_directionsIm[1][i] = metric[i] * (sgrad[i] / snorm - cgrad[i] / cnorm);
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
    m_coeffs.c1[k][0] /= chiSq;
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
      m_coeffs.s2[k][l] *= 1.0 / m_background;
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
      m_coeffs.c2[k][l] *= 2.0 / chiSq;
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
void MaxentData::calculateChisq() {

  size_t npoints = m_data.size();

  // Calculate
  // ChiSq = sum_i [ data_i - dataCalc_i ]^2 / [ error_i ]^2
  m_chisq = 0;
  for (size_t i = 0; i < npoints; i++) {
    if (m_errors[i] != 0.0) {
      double term = (m_data[i] - m_dataCalc[i]) / m_errors[i];
      m_chisq += term * term;
    }
  }
}

/**
* Transforms from image-space to data-space (Backward Fourier Transform)
* @param input : [input] A vector in image-space
* @return : The vector in data-space
*/
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

/**
* Transforms from data-space to image-space (Forward Fourier Transform)
* @param input : [input] A vector in data-space
* @return : The vector in image-space
*/
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
