// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_

#include "MantidAPI/FunctionGenerator.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Calculates crystal field spectrum.
*/
class DLLExport CrystalFieldSpectrum : public API::FunctionGenerator {
public:
  CrystalFieldSpectrum();
  std::string name() const override { return "CrystalFieldSpectrum"; }
  const std::string category() const override { return "General"; }
  void buildTargetFunction() const override;

protected:
  std::string writeToString(
      const std::string &parentLocalAttributesStr = "") const override;
  void updateTargetFunction() const override;

private:
  /// Number of fitted peaks in the spectrum.
  mutable size_t m_nPeaks;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_*/
