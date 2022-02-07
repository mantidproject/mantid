// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/MaxEnt/MaxentSpace.h"
#include "MantidAlgorithms/MaxEnt/MaxentTransform.h"

namespace Mantid {
namespace Algorithms {

/** MaxentTransformFourier : Defines a transformation from data space to image
  space (and vice-versa) where spaces are related by a **1D** Fourier Transform.
*/
class MANTID_ALGORITHMS_DLL MaxentTransformFourier : public MaxentTransform {
public:
  // Deleted default constructor
  MaxentTransformFourier() = delete;
  // Constructor
  MaxentTransformFourier(MaxentSpace_sptr dataSpace, MaxentSpace_sptr imageSpace);
  // Transfoms form image space to data space
  std::vector<double> imageToData(const std::vector<double> &image) override;
  // Transforms from data space to image space
  std::vector<double> dataToImage(const std::vector<double> &data) override;

private:
  MaxentSpace_sptr m_dataSpace;
  MaxentSpace_sptr m_imageSpace;
};

} // namespace Algorithms
} // namespace Mantid
