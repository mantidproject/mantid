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
* Transforms a 1D signal from image space to data space, performing an
* inverse Fast Fourier Transform. See also GSL documentation on FFT.
* Input is assumed real or complex according to the type of image space
* given to the constructor.
* Return value is real or complex according to the type of data space
* given to the constructor.
* If complex, input & return vectors consist of real part immediately
* followed by imaginary part of each individual value.
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

  /* Backward FT */
  gsl_fft_complex_wavetable *wavetable = gsl_fft_complex_wavetable_alloc(n / 2);
  gsl_fft_complex_workspace *workspace = gsl_fft_complex_workspace_alloc(n / 2);
  gsl_fft_complex_inverse(complexImage.data(), 1, n / 2, wavetable, workspace);
  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);

  return m_dataSpace->fromComplex(complexImage);
}

/**
* Transforms a 1D signal from data space to image space, performing a forward
* Fast Fourier Transform. See also GSL documentation on FFT.
* Input is assumed real or complex according to the type of data space
* given to the constructor.
* Return value is real or complex according to the type of image space
* given to the constructor.
* If complex, input & return vectors consist of real part immediately
* followed by imaginary part of each individual value.
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

  /*  Fourier transofrm */
  gsl_fft_complex_wavetable *wavetable = gsl_fft_complex_wavetable_alloc(n / 2);
  gsl_fft_complex_workspace *workspace = gsl_fft_complex_workspace_alloc(n / 2);
  gsl_fft_complex_forward(complexData.data(), 1, n / 2, wavetable, workspace);
  gsl_fft_complex_wavetable_free(wavetable);
  gsl_fft_complex_workspace_free(workspace);

  return m_imageSpace->fromComplex(complexData);
}

} // namespace Algorithms
} // namespace Mantid
