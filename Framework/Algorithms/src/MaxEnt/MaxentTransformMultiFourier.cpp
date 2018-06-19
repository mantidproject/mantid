#include "MantidAlgorithms/MaxEnt/MaxentTransformMultiFourier.h"
#include <boost/shared_array.hpp>
#include <gsl/gsl_fft_complex.h>

namespace Mantid {
namespace Algorithms {

/** Constructor */
MaxentTransformMultiFourier::MaxentTransformMultiFourier(MaxentSpace_sptr dataSpace,
                                               MaxentSpace_sptr imageSpace,
                                               MaxentSpace_sptr linearAdjustments,
                                               MaxentSpace_sptr constAdjustments)
    : MaxentTransformFourier(dataSpace, imageSpace), m_linearAdjustments(linearAdjustments), m_constAdjustments(constAdjustments) {}

/**
* Transforms a 1D signal from image space to data space, performing an
* a backward MaxentTransformFourier on it then creating a concatenated 
* copy of the resulting data for each spectrum and applying the adjustments 
* to them.
* Assumes complex input.
* @param image : [input] Image as a vector
* @return : The vector in the data space of concatenated spectra
*/
std::vector<double>
MaxentTransformMultiFourier::imageToData(const std::vector<double> &image) {

  return image;
}

/**
* Transforms a 1D signal from data space to image space, performing a forward
* Fast MexentTransformFourier on the sum of the spectra. Assumes complex
* input.
* @param data : [input] Data as a vector of concatenated spectra
* @return : The vector in the image space
*/
std::vector<double>
MaxentTransformMultiFourier::dataToImage(const std::vector<double> &data) {

  std::vector<double> complexData = m_dataSpace->toComplex(data);

  /* Performs forward Fourier transform */

  return data;
}

} // namespace Algorithms
} // namespace Mantid
