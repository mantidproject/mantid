// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidCurveFitting/EigenFortranDefs.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  CrystalFieldMoment is a function that calculates the induced
  magnetic moment (in bohr magnetons per ion, Am^2 or erg/Gauss) at some
  applied external magnetic field (in Tesla or Gauss) as a function of
  temperature (in Kelvin) for a particular crystal field splitting.
*/

class CrystalFieldMomentBase : public API::IFunction1D {
public:
  CrystalFieldMomentBase();
  void function1D(double *out, const double *xValues, const size_t nData) const override;

protected:
  mutable ComplexFortranMatrix m_ham;
  mutable int m_nre;
};

class MANTID_CURVEFITTING_DLL CrystalFieldMoment : public CrystalFieldPeaksBase, public CrystalFieldMomentBase {
public:
  CrystalFieldMoment();
  std::string name() const override { return "CrystalFieldMoment"; }
  const std::string category() const override { return "General"; }
  void setHamiltonian(const ComplexFortranMatrix &ham, const int nre);
  void function1D(double *out, const double *xValues, const size_t nData) const override;

private:
  bool m_setDirect;
};

class MANTID_CURVEFITTING_DLL CrystalFieldMomentCalculation : public API::ParamFunction, public CrystalFieldMomentBase {
public:
  CrystalFieldMomentCalculation();
  std::string name() const override { return "mt"; }
  const std::string category() const override { return "General"; }
  void setHamiltonian(const ComplexFortranMatrix &ham, const int nre);
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
