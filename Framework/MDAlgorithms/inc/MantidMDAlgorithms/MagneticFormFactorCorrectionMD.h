// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/MagneticIon.h"
#include "MantidMDAlgorithms/QTransform.h"

namespace Mantid {
namespace MDAlgorithms {

/** MagneticFormFactorCorrectionMD : TODO: DESCRIPTION
 */
class MANTID_MDALGORITHMS_DLL MagneticFormFactorCorrectionMD : public QTransform {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  double correction(const double) const override;

private:
  void init() override;
  void exec() override;
  Mantid::PhysicalConstants::MagneticIon ion;
};

} // namespace MDAlgorithms
} // namespace Mantid
