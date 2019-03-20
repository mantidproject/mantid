// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MaxEnt/MaxentTransformMultiFourier.h"
#include <gsl/gsl_fft_complex.h>

namespace Mantid {
namespace Algorithms {

/** Constructor */
MaxentTransformMultiFourier::MaxentTransformMultiFourier(
    MaxentSpaceComplex_sptr dataSpace, MaxentSpace_sptr imageSpace,
    size_t numSpec)
    : MaxentTransformFourier(dataSpace, imageSpace), m_numSpec(numSpec),
      m_linearAdjustments(), m_constAdjustments() {}

/**
 * Transforms a 1D signal from image space to data space, performing an
 * a backward MaxentTransformFourier on it then creating a concatenated
 * copy of the resulting data for each spectrum and applying the adjustments
 * to them.
 * Input is assumed real or complex according to the type of image space
 * given to the constructor.
 * Return value is real or complex according to the type of data space
 * given to the constructor.
 * If complex, input & return vectors consist of real part immediately
 * followed by imaginary part of each individual value.
 * @param image : [input] Image as a vector
 * @return : The vector in the data space of concatenated spectra
 */
std::vector<double>
MaxentTransformMultiFourier::imageToData(const std::vector<double> &image) {

  std::vector<double> dataOneSpec = MaxentTransformFourier::imageToData(image);

  // Create concatenated copies of transformed data (one for each spectrum)
  std::vector<double> data;
  data.reserve(m_numSpec * dataOneSpec.size());
  for (size_t s = 0; s < m_numSpec; s++) {
    for (const double &data_item : dataOneSpec) {
      data.emplace_back(data_item);
    }
  }

  // Apply adjustments (we assume there are sufficient adjustments supplied)
  double dataR = 0.123456789;
  double dataI = 0.987654321;
  if (!m_linearAdjustments.empty() && !m_constAdjustments.empty()) {
    for (size_t i = 0; i < data.size(); i++) {
      if (i % 2 == 0) { // Real part
        dataR = data[i];
        dataI = data[i + 1];
        data[i] = m_linearAdjustments[i] * dataR -
                  m_linearAdjustments[i + 1] * dataI + m_constAdjustments[i];
      } else { // Imaginary part
        data[i] = m_linearAdjustments[i] * dataR +
                  m_linearAdjustments[i - 1] * dataI + m_constAdjustments[i];
      }
    }
  } else if (!m_linearAdjustments.empty() && m_constAdjustments.empty()) {
    for (size_t i = 0; i < data.size(); i++) {
      if (i % 2 == 0) { // Real part
        dataR = data[i];
        dataI = data[i + 1];
        data[i] =
            m_linearAdjustments[i] * dataR - m_linearAdjustments[i + 1] * dataI;
      } else { // Imaginary part
        data[i] =
            m_linearAdjustments[i] * dataR + m_linearAdjustments[i - 1] * dataI;
      }
    }
  } else if (m_linearAdjustments.empty() && !m_constAdjustments.empty()) {
    for (size_t i = 0; i < data.size(); i++) {
      data[i] = data[i] + m_constAdjustments[i];
    }
  }

  return data;
}

/**
 * Transforms a 1D signal from data space to image space, performing a forward
 * Fast MexentTransformFourier on the sum of the spectra.
 * Input is assumed real or complex according to the type of data space
 * given to the constructor.
 * Return value is real or complex according to the type of image space
 * given to the constructor.
 * If complex, input & return vectors consist of real part immediately
 * followed by imaginary part of each individual value.
 * @param data : [input] Data as a vector of concatenated spectra
 * @return : The vector in the image space
 */
std::vector<double>
MaxentTransformMultiFourier::dataToImage(const std::vector<double> &data) {

  if (data.size() % m_numSpec)
    throw std::invalid_argument(
        "Size of data vector must be a multiple of number of spectra.");

  // Sum the concatenated spectra in data
  size_t nData = data.size() / (m_numSpec);
  std::vector<double> dataSum(nData, 0.0);
  for (size_t s = 0; s < m_numSpec; s++) {
    for (size_t i = 0; i < nData; i++) {
      dataSum[i] += data[s * nData + i];
    }
  }
  // Then apply forward fourier transform to sum
  std::vector<double> image = MaxentTransformFourier::dataToImage(dataSum);

  return image;
}

/**
 * Sets the adjustments to be applied to the data when converted from image.
 * @param linAdj : [input] Linear adjustments as complex numbers for all spectra
 * concatenated
 * @param constAdj: [input] Constant adjustments as complex numbers for all
 * spectra concatenated
 */
void MaxentTransformMultiFourier::setAdjustments(
    const std::vector<double> &linAdj, const std::vector<double> &constAdj) {
  m_linearAdjustments = linAdj;
  m_constAdjustments = constAdj;
}

} // namespace Algorithms
} // namespace Mantid
