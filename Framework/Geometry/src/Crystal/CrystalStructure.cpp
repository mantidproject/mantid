#include "MantidGeometry/Crystal/CrystalStructure.h"
#include <algorithm>
#include <boost/bind.hpp>
#include <stdexcept>

#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/StructureFactorCalculatorSummation.h"

#include <iomanip>
#include <iostream>

namespace Mantid {
namespace Geometry {

using namespace Mantid::Kernel;

/// SpaceGroup/Scatterers constructor
CrystalStructure::CrystalStructure(
    const UnitCell &unitCell, const SpaceGroup_const_sptr &spaceGroup,
    const CompositeBraggScatterer_sptr &scatterers) {
  initializeScatterers();

  addScatterers(scatterers);
  setCell(unitCell);
  setSpaceGroup(spaceGroup);
}

/// String-based constructor
CrystalStructure::CrystalStructure(const std::string &unitCellString,
                                   const std::string &spaceGroupString,
                                   const std::string &scattererString) {
  initializeScatterers();

  addScatterers(CompositeBraggScatterer::create(
      IsotropicAtomBraggScattererParser(scattererString)()));
  setCell(strToUnitCell(unitCellString));
  setSpaceGroup(
      SpaceGroupFactory::Instance().createSpaceGroup(spaceGroupString));
}

/// Returns the unit cell of the structure
UnitCell CrystalStructure::cell() const { return m_cell; }

/// Assigns a new unit cell
void CrystalStructure::setCell(const UnitCell &cell) {
  m_cell = cell;

  assignUnitCellToScatterers(m_cell);
}

/// Returns the space group of the crystal structure
SpaceGroup_const_sptr CrystalStructure::spaceGroup() const {
  return m_spaceGroup;
}

/**
 * Assigns a new space group to the crystal structure
 *
 * @param spaceGroup :: New space group of the crystal structure
 */
void CrystalStructure::setSpaceGroup(const SpaceGroup_const_sptr &spaceGroup) {
  m_spaceGroup = spaceGroup;

  setReflectionConditionFromSpaceGroup(m_spaceGroup);
}

/// Return a clone of the internal CompositeBraggScatterer instance.
CompositeBraggScatterer_sptr CrystalStructure::getScatterers() const {
  BraggScatterer_sptr clone = m_scatterers->clone();

  return boost::dynamic_pointer_cast<CompositeBraggScatterer>(clone);
}

/// Remove all scatterers and set the supplied ones as new scatterers.
void CrystalStructure::setScatterers(
    const CompositeBraggScatterer_sptr &scatterers) {
  m_scatterers->removeAllScatterers();

  addScatterers(scatterers);
}

/// Adds all scatterers in the supplied collection into the internal one
/// (scatterers are copied).
void CrystalStructure::addScatterers(
    const CompositeBraggScatterer_sptr &scatterers) {
  size_t count = scatterers->nScatterers();

  for (size_t i = 0; i < count; ++i) {
    m_scatterers->addScatterer(scatterers->getScatterer(i));
  }

  assignUnitCellToScatterers(m_cell);
}

/// Tries to set the centering from the space group symbol or removes the
/// current centering if creation fails.
void CrystalStructure::setReflectionConditionFromSpaceGroup(
    const SpaceGroup_const_sptr &spaceGroup) {
  m_centering.reset();

  // First letter is centering
  std::string centering = spaceGroup->hmSymbol().substr(0, 1);

  if (centering == "R") {
    centering = "Robv";
  }

  std::vector<ReflectionCondition_sptr> reflectionConditions =
      getAllReflectionConditions();
  for (auto &reflectionCondition : reflectionConditions) {
    if (reflectionCondition->getSymbol() == centering) {
      m_centering = reflectionCondition;
      break;
    }
  }
}

/// Assigns the cell to all scatterers
void CrystalStructure::assignUnitCellToScatterers(const UnitCell &unitCell) {
  if (!m_scatterers) {
    throw std::runtime_error(
        "Scatterer collection is a null pointer. Aborting.");
  }

  if (m_scatterers->existsProperty("UnitCell")) {
    m_scatterers->setProperty("UnitCell", unitCellToStr(unitCell));
  }
}

/// Initializes the internal storage for scatterers
void CrystalStructure::initializeScatterers() {
  if (!m_scatterers) {
    m_scatterers = CompositeBraggScatterer::create();
  }
}

} // namespace Geometry
} // namespace Mantid
