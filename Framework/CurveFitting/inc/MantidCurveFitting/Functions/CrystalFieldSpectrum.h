// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionGenerator.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Calculates crystal field spectrum.
*/
class MANTID_CURVEFITTING_DLL CrystalFieldSpectrum : public API::FunctionGenerator {
public:
  CrystalFieldSpectrum();

  void init() override;
  std::string name() const override { return "CrystalFieldSpectrum"; }
  const std::string category() const { return "General"; }
  void buildTargetFunction() const override;

protected:
  std::string writeToString(const std::string &parentLocalAttributesStr = "") const override;
  void updateTargetFunction() const override;

private:
  /// Number of fitted peaks in the spectrum.
  mutable size_t m_nPeaks;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
