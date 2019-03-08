// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MATERIALBUILDER_H_
#define MANTID_KERNEL_MATERIALBUILDER_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Material.h"
#include <boost/optional/optional.hpp>

namespace Mantid {
// Forward declare
namespace PhysicalConstants {
struct NeutronAtom;
}

namespace Kernel {

/**
  Create a material from a set of user defined options.
*/
class MANTID_KERNEL_DLL MaterialBuilder {
public:
  enum class NumberDensityUnit { Atoms, FormulaUnits };
  MaterialBuilder();

  MaterialBuilder &setName(const std::string &name);

  MaterialBuilder &setFormula(const std::string &formula);
  MaterialBuilder &setAtomicNumber(int atomicNumber);
  MaterialBuilder &setMassNumber(int massNumber);

  MaterialBuilder &setNumberDensity(double rho);
  MaterialBuilder &setNumberDensityUnit(NumberDensityUnit unit);
  MaterialBuilder &setZParameter(double zparam);
  MaterialBuilder &setUnitCellVolume(double cellVolume);
  MaterialBuilder &setMassDensity(double massDensity);

  MaterialBuilder &setTotalScatterXSection(double xsec);
  MaterialBuilder &setCoherentXSection(double xsec);
  MaterialBuilder &setIncoherentXSection(double xsec);
  MaterialBuilder &setAbsorptionXSection(double xsec);

  Material build() const;

private:
  using Composition = std::tuple<PhysicalConstants::NeutronAtom, double>;

  bool hasOverrideNeutronProperties() const;
  void overrideNeutronProperties(PhysicalConstants::NeutronAtom &neutron) const;
  PhysicalConstants::NeutronAtom generateCustomNeutron() const;

  // Material::ChemicalFormula createCompositionFromFormula() const;
  Material::ChemicalFormula createCompositionFromAtomicNumber() const;
  double getOrCalculateRho(const Material::ChemicalFormula &formula) const;

  std::string m_name;
  Material::ChemicalFormula m_formula;
  boost::optional<int> m_atomicNo;
  int m_massNo;
  boost::optional<double> m_numberDensity, m_zParam, m_cellVol, m_massDensity;
  boost::optional<double> m_totalXSection, m_cohXSection, m_incXSection,
      m_absSection;
  NumberDensityUnit m_numberDensityUnit;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MATERIALBUILDER_H_ */
