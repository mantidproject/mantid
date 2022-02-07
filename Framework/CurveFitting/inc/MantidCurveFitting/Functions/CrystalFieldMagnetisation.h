// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  CrystalFieldMagnetisation is a function that calculates the induced
  magnetic moment (in bohr magnetons per ion, Am^2 or erg/Gauss) as a function
  of applied external magnetic field (in Tesla or Gauss), for a particular
  crystal field splitting.
*/

class CrystalFieldMagnetisationBase : public API::IFunction1D {
public:
  CrystalFieldMagnetisationBase();
  void function1D(double *out, const double *xValues, const size_t nData) const override;

protected:
  mutable ComplexFortranMatrix m_ham;
  mutable int m_nre;
};

class MANTID_CURVEFITTING_DLL CrystalFieldMagnetisation : public CrystalFieldPeaksBase,
                                                          public CrystalFieldMagnetisationBase {
public:
  CrystalFieldMagnetisation();
  std::string name() const override { return "CrystalFieldMagnetisation"; }
  const std::string category() const override { return "General"; }
  void setHamiltonian(const ComplexFortranMatrix &ham, const int nre);
  void function1D(double *out, const double *xValues, const size_t nData) const override;

private:
  bool m_setDirect;
};

class MANTID_CURVEFITTING_DLL CrystalFieldMagnetisationCalculation : public API::ParamFunction,
                                                                     public CrystalFieldMagnetisationBase {
public:
  CrystalFieldMagnetisationCalculation();
  std::string name() const override { return "mh"; }
  const std::string category() const override { return "General"; }
  void setHamiltonian(const ComplexFortranMatrix &ham, const int nre);
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
