// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMDAlgorithms/QTransform.h"

namespace Mantid {
namespace MDAlgorithms {

/** DebyeWallerCorrectionMD : Correct event signal and error values for Debye-Waller factor.
 */
class MANTID_MDALGORITHMS_DLL DebyeWallerFactorCorrectionMD : public QTransform {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  double correction(const double) const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid
