// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Material.h"
#include <optional>

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
  MaterialBuilder &setEffectiveNumberDensity(double rho_eff);
  MaterialBuilder &setPackingFraction(double fraction);
  MaterialBuilder &setZParameter(double zparam);
  MaterialBuilder &setUnitCellVolume(double cellVolume);
  MaterialBuilder &setMassDensity(double massDensity);

  MaterialBuilder &setTotalScatterXSection(double xsec);
  MaterialBuilder &setCoherentXSection(double xsec);
  MaterialBuilder &setIncoherentXSection(double xsec);
  MaterialBuilder &setAbsorptionXSection(double xsec);
  MaterialBuilder &setAttenuationProfileFilename(const std::string &filename);
  MaterialBuilder &setXRayAttenuationProfileFilename(const std::string &filename);

  void setAttenuationSearchPath(std::string path);

  Material build() const;

private:
  using Composition = std::tuple<PhysicalConstants::NeutronAtom, double>;

  bool hasOverrideNeutronProperties() const;
  void overrideNeutronProperties(PhysicalConstants::NeutronAtom &neutron) const;
  PhysicalConstants::NeutronAtom generateCustomNeutron() const;

  // Material::ChemicalFormula createCompositionFromFormula() const;
  Material::ChemicalFormula createCompositionFromAtomicNumber() const;

  struct density_packing {
    double number_density;
    double effective_number_density;
    double packing_fraction;
  };
  density_packing getOrCalculateRhoAndPacking(const Material::ChemicalFormula &formula) const;

  std::string m_name;
  Material::ChemicalFormula m_formula;
  std::optional<int> m_atomicNo;
  int m_massNo;
  std::optional<double> m_numberDensity, m_packingFraction;
  std::optional<double> m_numberDensityEff;
  std::optional<double> m_zParam, m_cellVol, m_massDensity;
  std::optional<double> m_totalXSection, m_cohXSection, m_incXSection, m_absSection;
  NumberDensityUnit m_numberDensityUnit;
  std::optional<std::string> m_attenuationProfileFileName;
  std::optional<std::string> m_xRayAttenuationProfileFileName;
  std::string m_attenuationFileSearchPath;
};

} // namespace Kernel
} // namespace Mantid
