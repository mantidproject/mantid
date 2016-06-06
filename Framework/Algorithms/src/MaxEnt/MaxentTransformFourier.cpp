#include "MantidAlgorithms/MaxEnt/MaxentTransformFourier.h"
#include <boost/shared_array.hpp>
#include <gsl/gsl_fft_complex.h>

namespace Mantid {
namespace Algorithms {

/** Constructor */
MaxentTransformFourier::MaxentTransformFourier(MaxentSpace_sptr dataSpace,
                                               MaxentSpace_sptr imageSpace)
    : m_dataSpace(dataSpace), m_imageSpace(imageSpace) {}

/**
* Transforms a 1D signal from image space to data space, performing a backward
* Fourier Transform. Assumes complex input.
* @param image : [input] Image as a vector
* @return : The vector in the data space
*/
std::vector<double>
MaxentTransformFourier::imageToData(const std::vector<double> &image) {

  std::vector<double> complexImage = m_imageSpace->toComplex(image);

  /* Performs backward Fourier transform */

  size_t n = complexImage.size();

  if (n % 2) {
    throw std::invalid_argument("Cannot transform to data space");
  }

  /* Prepare the data */
  boost::shared_array<double> result(new double[n]);
  for (size_t i = 0; i < n; i++) {
    result[i] = complexImage[i];
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

  return m_dataSpace->fromComplex(output);
}

/**
* Transforms a 1D signal from data space to image space, performing a forward
* Fourier Transform. Assumes complex input.
* @param data : [input] Data as a vector
* @return : The vector in the image space
*/
std::vector<double>
MaxentTransformFourier::dataToImage(const std::vector<double> &data) {

  std::vector<double> complexData = m_dataSpace->toComplex(data);

  /* Performs forward Fourier transform */

  size_t n = complexData.size();

  if (n % 2) {
    throw std::invalid_argument("Cannot transform to image space");
  }

  /* Prepare the data */
  boost::shared_array<double> result(new double[n]);
  for (size_t i = 0; i < n; i++) {
    result[i] = complexData[i];
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

  return m_imageSpace->fromComplex(output);
}

} // namespace Algorithms
} // namespace Mantid
