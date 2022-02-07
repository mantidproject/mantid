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
  CrystalFieldSusceptibility is a function that calculates the molar magnetic
  susceptibility (in cm^3/mol or m^3/mol) due to the crystalline electric field.
*/

class CrystalFieldSusceptibilityBase : public API::IFunction1D {
public:
  CrystalFieldSusceptibilityBase();
  void function1D(double *out, const double *xValues, const size_t nData) const override;

protected:
  mutable DoubleFortranVector m_en;
  mutable ComplexFortranMatrix m_wf;
  mutable int m_nre;
};

class MANTID_CURVEFITTING_DLL CrystalFieldSusceptibility : public CrystalFieldPeaksBase,
                                                           public CrystalFieldSusceptibilityBase {
public:
  CrystalFieldSusceptibility();
  std::string name() const override { return "CrystalFieldSusceptibility"; }
  const std::string category() const override { return "General"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void setEigensystem(const DoubleFortranVector &en, const ComplexFortranMatrix &wf, const int nre);

private:
  bool m_setDirect;
};

class MANTID_CURVEFITTING_DLL CrystalFieldSusceptibilityCalculation : public API::ParamFunction,
                                                                      public CrystalFieldSusceptibilityBase {
public:
  CrystalFieldSusceptibilityCalculation();
  std::string name() const override { return "chi"; }
  const std::string category() const override { return "General"; }
  void setEigensystem(const DoubleFortranVector &en, const ComplexFortranMatrix &wf, const int nre);
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
