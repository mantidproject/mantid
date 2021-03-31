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
  CrystalFieldHeatCapacity is a function that calculates the molar magnetic
  heat capacity (in J/K/mol) due to the splitting of electronic energy levels
  due to the crystal field.
*/

class CrystalFieldHeatCapacityBase : public API::IFunction1D {
public:
  CrystalFieldHeatCapacityBase();
  void function1D(double *out, const double *xValues, const size_t nData) const override;

protected:
  mutable DoubleFortranVector m_en;
};

class MANTID_CURVEFITTING_DLL CrystalFieldHeatCapacity : public CrystalFieldPeaksBase,
                                                         public CrystalFieldHeatCapacityBase {
public:
  CrystalFieldHeatCapacity();
  std::string name() const override { return "CrystalFieldHeatCapacity"; }
  const std::string category() const override { return "General"; }
  void setEnergy(const DoubleFortranVector &en);
  void function1D(double *out, const double *xValues, const size_t nData) const override;

private:
  bool m_setDirect;
};

class MANTID_CURVEFITTING_DLL CrystalFieldHeatCapacityCalculation : public API::ParamFunction,
                                                                    public CrystalFieldHeatCapacityBase {
public:
  CrystalFieldHeatCapacityCalculation();
  std::string name() const override { return "cv"; }
  const std::string category() const override { return "General"; }
  void setEnergy(const DoubleFortranVector &en);
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
