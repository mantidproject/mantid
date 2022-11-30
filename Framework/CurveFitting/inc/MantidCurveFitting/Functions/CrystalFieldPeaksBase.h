// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/EigenFortranDefs.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  CrystalFieldPeaks is a function that calculates crystal field peak
  positions and intensities.
*/
class MANTID_CURVEFITTING_DLL CrystalFieldPeaksBase : public API::ParamFunction {
public:
  CrystalFieldPeaksBase();
  void setAttribute(const std::string &name, const Attribute &) override;

  /// Calculate the crystal field eigensystem
  void calculateEigenSystem(DoubleFortranVector &en, ComplexFortranMatrix &wf, ComplexFortranMatrix &ham,
                            ComplexFortranMatrix &hz, int &nre) const;
  inline void calculateEigenSystem(DoubleFortranVector &en, ComplexFortranMatrix &wf, int &nre) const {
    ComplexFortranMatrix ham, hz;
    calculateEigenSystem(en, wf, ham, hz, nre);
  }

protected:
  /// Store the default domain size after first
  /// function evaluation
  mutable size_t m_defaultDomainSize;
};

class MANTID_CURVEFITTING_DLL CrystalFieldPeaksBaseImpl : public CrystalFieldPeaksBase {
public:
  std::string name() const override;
  void function(const API::FunctionDomain &, API::FunctionValues &) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
