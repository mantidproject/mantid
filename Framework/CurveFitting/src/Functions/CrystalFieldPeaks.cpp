// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Functions/CrystalElectricField.h"

#include <functional>
#include <map>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(CrystalFieldPeaks)

/// Constructor
CrystalFieldPeaks::CrystalFieldPeaks()
    : CrystalFieldPeaksBase(), API::IFunctionGeneral(), m_defaultDomainSize(0) {

  declareAttribute("Temperature", Attribute(1.0));
  declareParameter("IntensityScaling", 1.0,
                   "A scaling factor for peak intensities.");
}

std::string CrystalFieldPeaks::name() const { return "CrystalFieldPeaks"; }

size_t CrystalFieldPeaks::getNumberDomainColumns() const { return 0; }

size_t CrystalFieldPeaks::getNumberValuesPerArgument() const { return 2; }

size_t CrystalFieldPeaks::getDefaultDomainSize() const {
  return m_defaultDomainSize;
}

void CrystalFieldPeaks::functionGeneral(
    const API::FunctionDomainGeneral & /*domain*/,
    API::FunctionValues &values) const {

  DoubleFortranVector en;
  ComplexFortranMatrix wf;
  int nre = 0;
  calculateEigenSystem(en, wf, nre);

  auto temperature = getAttribute("Temperature").asDouble();
  IntFortranVector degeneration;
  DoubleFortranVector eEnergies;
  DoubleFortranMatrix iEnergies;
  const double de = getAttribute("ToleranceEnergy").asDouble();
  const double di = getAttribute("ToleranceIntensity").asDouble();
  calculateIntensities(nre, en, wf, temperature, de, degeneration, eEnergies,
                       iEnergies);

  DoubleFortranVector eExcitations;
  DoubleFortranVector iExcitations;
  calculateExcitations(eEnergies, iEnergies, de, di, eExcitations,
                       iExcitations);

  size_t n = eExcitations.size();
  if (2 * n > values.size()) {
    values.expand(2 * n);
  }

  m_defaultDomainSize = n;
  double scaling = getParameter("IntensityScaling");

  for (size_t i = 0; i < n; ++i) {
    values.setCalculated(i, eExcitations.get(i));
    values.setCalculated(i + n, iExcitations.get(i) * scaling);
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
