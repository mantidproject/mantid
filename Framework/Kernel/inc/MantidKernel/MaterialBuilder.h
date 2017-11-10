#ifndef MANTID_KERNEL_MATERIALBUILDER_H_
#define MANTID_KERNEL_MATERIALBUILDER_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Material.h"
#include <boost/optional/optional.hpp>
#include <string>
#include <tuple>

namespace Mantid {
// Forward declare
namespace PhysicalConstants {
struct NeutronAtom;
}

namespace Kernel {

/**
  Create a material from a set of user defined options.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL MaterialBuilder {
public:
  MaterialBuilder();

  MaterialBuilder &setName(const std::string &name);

  MaterialBuilder &setFormula(const std::string &formula);
  MaterialBuilder &setAtomicNumber(int atomicNumber);
  MaterialBuilder &setMassNumber(int massNumber);

  MaterialBuilder &setNumberDensity(double rho);
  MaterialBuilder &setZParameter(double zparam);
  MaterialBuilder &setUnitCellVolume(double cellVolume);
  MaterialBuilder &setMassDensity(double massDensity);

  MaterialBuilder &setTotalScatterXSection(double xsec);
  MaterialBuilder &setCoherentXSection(double xsec);
  MaterialBuilder &setIncoherentXSection(double xsec);
  MaterialBuilder &setAbsorptionXSection(double xsec);

  Material build() const;

private:
  typedef std::tuple<PhysicalConstants::NeutronAtom, double> Composition;

  bool hasOverrideNeutronProperties() const;
  void overrideNeutronProperties(PhysicalConstants::NeutronAtom &neutron) const;
  PhysicalConstants::NeutronAtom generateCustomNeutron() const;

  // Material::ChemicalFormula createCompositionFromFormula() const;
  Material::ChemicalFormula createCompositionFromAtomicNumber() const;
  double getOrCalculateRho(const Material::ChemicalFormula &formula) const;

  std::string m_name;
  std::unique_ptr<Material::ChemicalFormula> m_formula;
  boost::optional<int> m_atomicNo;
  int m_massNo;
  boost::optional<double> m_numberDensity, m_zParam, m_cellVol, m_massDensity;
  boost::optional<double> m_totalXSection, m_cohXSection, m_incXSection,
      m_absSection;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MATERIALBUILDER_H_ */
