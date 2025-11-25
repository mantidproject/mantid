// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/StructureFactorCalculatorSummation.h"
#include "MantidGeometry/Crystal/BraggScattererInCrystalStructure.h"

#include <iomanip>

namespace Mantid::Geometry {

using namespace Kernel;

StructureFactorCalculatorSummation::StructureFactorCalculatorSummation()
    : StructureFactorCalculator(), m_unitCellScatterers(CompositeBraggScatterer::create()) {}

/// Returns the structure factor obtained from the stored scatterers.
StructureFactor StructureFactorCalculatorSummation::getF(const Kernel::V3D &hkl) const {
  return m_unitCellScatterers->calculateStructureFactor(hkl);
}

/// Calls updateUnitCellScatterers() to rebuild the complete list of scatterers.
void StructureFactorCalculatorSummation::crystalStructureSetHook(const CrystalStructure &crystalStructure) {
  updateUnitCellScatterers(crystalStructure);
}

/**
 * Rebuilds the internal list of scatterers
 *
 * This method extracts two items from the supplied CrystalStructure object, the
 * list of scatterers in the asymmetric unit and the space group. Then it
 * generates all equivalent positions and creates a complete list of scatterers.
 *
 * @param crystalStructure :: CrystalStructure for structure factor calculation.
 */
void StructureFactorCalculatorSummation::updateUnitCellScatterers(const CrystalStructure &crystalStructure) {
  m_unitCellScatterers->removeAllScatterers();

  CompositeBraggScatterer_sptr scatterersInAsymmetricUnit = crystalStructure.getScatterers();
  SpaceGroup_const_sptr spaceGroup = crystalStructure.spaceGroup();

  if (spaceGroup) {
    std::vector<BraggScatterer_sptr> braggScatterers;
    braggScatterers.reserve(scatterersInAsymmetricUnit->nScatterers() * spaceGroup->order());

    for (size_t i = 0; i < scatterersInAsymmetricUnit->nScatterers(); ++i) {
      BraggScattererInCrystalStructure_sptr current =
          std::dynamic_pointer_cast<BraggScattererInCrystalStructure>(scatterersInAsymmetricUnit->getScatterer(i));

      if (current) {
        std::vector<V3D> positions = spaceGroup->getEquivalentPositions(current->getPosition());

        for (auto const &position : positions) {
          BraggScatterer_sptr clone = current->clone();
          clone->setProperty("Position", getV3DasString(position));

          braggScatterers.emplace_back(clone);
        }
      }
    }

    m_unitCellScatterers->setScatterers(braggScatterers);
  }
}

/// Return V3D as string without losing precision.
std::string StructureFactorCalculatorSummation::getV3DasString(const V3D &point) const {
  std::ostringstream posStream;
  posStream << std::setprecision(17);
  posStream << point;

  return posStream.str();
}

} // namespace Mantid::Geometry
