// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/MaxEnt/MaxentSpace.h"
#include "MantidAlgorithms/MaxEnt/MaxentSpaceComplex.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransformFourier.h"

namespace Mantid {
namespace Algorithms {

using MaxentSpace_sptr = std::shared_ptr<MaxentSpace>;
using MaxentSpaceComplex_sptr = std::shared_ptr<MaxentSpaceComplex>;

/** MaxentTransformMultiFourier : Defines a transformation from
  data space to image space (and vice-versa)
  where spaces are related by a **1D** Fourier Transform,
  in which which the data has multiple spectra concatenatenated.

  In transforming from data to image, the spectra are added together
  before tranforming to a single image.
  In transforming the image to data, copies to the transformed data
  (one for each spectrum) are concatenated and then have the supplied
  adjustments applied.

  The concatenated format of the data is chosen to enable existing code
  to calculate its chi squared.
*/
class MANTID_ALGORITHMS_DLL MaxentTransformMultiFourier : public MaxentTransformFourier {
public:
  // Deleted default constructor
  MaxentTransformMultiFourier() = delete;
  // Constructor
  MaxentTransformMultiFourier(const MaxentSpaceComplex_sptr &dataSpace, MaxentSpace_sptr imageSpace, size_t numSpec);
  // Transfoms form image space to data space
  std::vector<double> imageToData(const std::vector<double> &image) override;
  // Transforms from data space to image space
  std::vector<double> dataToImage(const std::vector<double> &data) override;
  // Set the adjustments to be applie to data when converted from image
  void setAdjustments(const std::vector<double> &linAdj, const std::vector<double> &constAdj);

private:
  MaxentSpace_sptr m_dataSpace;
  MaxentSpace_sptr m_imageSpace;
  size_t m_numSpec;
  std::vector<double> m_linearAdjustments;
  std::vector<double> m_constAdjustments;
};

} // namespace Algorithms
} // namespace Mantid
