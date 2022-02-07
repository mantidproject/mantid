// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunctionGeneral.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  CrystalFieldPeaks is a function that calculates crystal field peak
  positions and intensities.
*/
class MANTID_CURVEFITTING_DLL CrystalFieldPeaks : public CrystalFieldPeaksBase, public API::IFunctionGeneral {
public:
  CrystalFieldPeaks();
  std::string name() const override;
  size_t getNumberDomainColumns() const override;
  size_t getNumberValuesPerArgument() const override;
  void functionGeneral(const API::FunctionDomainGeneral &generalDomain, API::FunctionValues &values) const override;
  size_t getDefaultDomainSize() const override;

private:
  /// Store the default domain size after first
  /// function evaluation
  mutable size_t m_defaultDomainSize;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
