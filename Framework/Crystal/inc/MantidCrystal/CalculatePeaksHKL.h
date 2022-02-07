// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {

/** CalculatePeaksHKL : Calculate the HKL value for each peak without any
 rounding or optimization
 *  of the UB Matrix.
*/
class MANTID_CRYSTAL_DLL CalculatePeaksHKL : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates Miller indices for each peak. No rounding or UB "
           "optimization.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"AddPeak"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid
