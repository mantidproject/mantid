// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/PoldiCreatePeaksFromCell.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"

namespace Mantid::Poldi {

using API::ITableWorkspace;
using API::WorkspaceProperty;
using Kernel::Direction;
using namespace Geometry;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiCreatePeaksFromCell)

const std::string PoldiCreatePeaksFromCell::name() const { return "PoldiCreatePeaksFromCell"; }

//----------------------------------------------------------------------------------------------

/// Algorithm's version for identification. @see Algorithm::version
int PoldiCreatePeaksFromCell::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiCreatePeaksFromCell::category() const { return "SINQ\\Poldi"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PoldiCreatePeaksFromCell::summary() const {
  return "Generate a TableWorkspace with all symmetry independent reflections "
         "using a unit cell.";
}

std::map<std::string, std::string> PoldiCreatePeaksFromCell::validateInputs() {
  std::map<std::string, std::string> errorMap;

  double dMin = getProperty("LatticeSpacingMin");
  const Property *dMaxProperty = getProperty("LatticeSpacingMax");

  auto dMax = boost::lexical_cast<double>(dMaxProperty->value());
  if (!dMaxProperty->isDefault() && (dMax < dMin)) {
    errorMap["LatticeSpacingMax"] = std::string("LatticeSpacingMax is less than LatticeSpacingMin.");
  }

  return errorMap;
}

/// Tries to construct a space group object using the space group factory.
SpaceGroup_const_sptr PoldiCreatePeaksFromCell::getSpaceGroup(const std::string &spaceGroupString) const {
  return SpaceGroupFactory::Instance().createSpaceGroup(spaceGroupString);
}

/** Returns the largest lattice spacing based on the algorithm properties
 *
 *  This method returns the largest allowed lattice spacing for calculations. If
 *the
 *  user has not supplied a value for LatticeSpacingMax, this value is
 *determined from the UnitCell-object.
 *  The largest possible spacing is equal to the largest cell edge. To avoid
 *problems
 *  with floating point comparison and different use of < and <=, 1.0 is added
 *to this value.
 *
 *  If LatticeSpacingMax is not default, this value is used, no matter if it's
 *larger or smaller than
 *  the maximum determined by the cell.
 *
 *  @param unitCell :: Unit cell which determines the limit
 *  @return Largest considered lattice spacing
 */
double PoldiCreatePeaksFromCell::getDMaxValue(const UnitCell &unitCell) const {
  const Property *dMaxProperty = getProperty("LatticeSpacingMax");

  if (dMaxProperty->isDefault()) {
    // Instead of returning just the value, 1.0 is added to avoid running into
    // problems with comparison operators
    return getLargestDValue(unitCell) + 1.0;
  }

  return getProperty("LatticeSpacingMax");
}

/// Returns the largest possible lattice spacing for the given cell.
double PoldiCreatePeaksFromCell::getLargestDValue(const UnitCell &unitCell) const {
  return std::max(std::max(unitCell.a(), unitCell.b()), unitCell.c());
}

/// Constructs a UnitCell-object from the algorithm properties.
UnitCell PoldiCreatePeaksFromCell::getUnitCellFromProperties() const {
  double a = getProperty("a");
  double b = getProperty("b");
  double c = getProperty("c");

  double alpha = getProperty("alpha");
  double beta = getProperty("beta");
  double gamma = getProperty("gamma");

  return UnitCell(a, b, c, alpha, beta, gamma);
}

/** Returns a new UnitCell-object with crystal system constraints taken into
 *  account
 *
 *  This method constructs a new UnitCell-object based on the values of the
 *  supplied cell,
 *  but takes into account the constraints of the crystal system. For
 *  monoclinic, a unique b-axis is assumed.
 *
 *  It's useful for "cleaning" user input.
 *
 * @param unitCell :: UnitCell-object which should be constrained
 * @param crystalSystem :: Crystal system which is used for constraints
 * @return UnitCell-object with applied constraints
 */
UnitCell PoldiCreatePeaksFromCell::getConstrainedUnitCell(const UnitCell &unitCell,
                                                          const PointGroup::CrystalSystem &crystalSystem,
                                                          const Group::CoordinateSystem &coordinateSystem) const {
  switch (crystalSystem) {
  case PointGroup::CrystalSystem::Cubic:
    return UnitCell(unitCell.a(), unitCell.a(), unitCell.a());
  case PointGroup::CrystalSystem::Tetragonal:
    return UnitCell(unitCell.a(), unitCell.a(), unitCell.c());
  case PointGroup::CrystalSystem::Orthorhombic:
    return UnitCell(unitCell.a(), unitCell.b(), unitCell.c());
  case PointGroup::CrystalSystem::Monoclinic:
    return UnitCell(unitCell.a(), unitCell.b(), unitCell.c(), 90.0, unitCell.beta(), 90.0);
  case PointGroup::CrystalSystem::Trigonal:
    if (coordinateSystem == Group::Orthogonal) {
      return UnitCell(unitCell.a(), unitCell.a(), unitCell.a(), unitCell.alpha(), unitCell.alpha(), unitCell.alpha());
    }
  // fall through to hexagonal.
  case PointGroup::CrystalSystem::Hexagonal:
    return UnitCell(unitCell.a(), unitCell.a(), unitCell.c(), 90.0, 90.0, 120.0);
  default:
    return UnitCell(unitCell);
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PoldiCreatePeaksFromCell::init() {
  std::vector<std::string> spaceGroups = SpaceGroupFactory::Instance().subscribedSpaceGroupSymbols();
  declareProperty("SpaceGroup", spaceGroups.front(), std::make_shared<StringListValidator>(spaceGroups),
                  "SpaceGroup of the crystal structure.");

  declareProperty("Atoms", "",
                  "Atoms in the asymmetric unit. Format: \n"
                  "Element x y z Occupancy U; ... ");

  std::shared_ptr<BoundedValidator<double>> latticeParameterEdgeValidator =
      std::make_shared<BoundedValidator<double>>(0.0, 0.0);
  latticeParameterEdgeValidator->clearUpper();
  declareProperty("a", 1.0, latticeParameterEdgeValidator, "Lattice parameter a");
  declareProperty("b", 1.0, latticeParameterEdgeValidator->clone(), "Lattice parameter b");
  declareProperty("c", 1.0, latticeParameterEdgeValidator->clone(), "Lattice parameter c");

  std::shared_ptr<BoundedValidator<double>> latticeParameterAngleValidator =
      std::make_shared<BoundedValidator<double>>(0.0, 180.0);
  declareProperty("alpha", 90.0, latticeParameterAngleValidator, "Lattice parameter alpha");
  declareProperty("beta", 90.0, latticeParameterAngleValidator->clone(), "Lattice parameter beta");
  declareProperty("gamma", 90.0, latticeParameterAngleValidator->clone(), "Lattice parameter gamma");

  std::shared_ptr<BoundedValidator<double>> dValidator = std::make_shared<BoundedValidator<double>>(0.01, 0.0);
  dValidator->clearUpper();

  declareProperty("LatticeSpacingMin", 0.5, dValidator, "Smallest allowed lattice spacing.");
  declareProperty("LatticeSpacingMax", 0.0, "Largest allowed lattice spacing.");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "List with calculated peaks.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PoldiCreatePeaksFromCell::exec() {
  // Get all user input regarding the unit cell
  SpaceGroup_const_sptr spaceGroup = getSpaceGroup(getProperty("SpaceGroup"));
  PointGroup_sptr pointGroup = PointGroupFactory::Instance().createPointGroupFromSpaceGroup(spaceGroup);
  UnitCell unitCell = getConstrainedUnitCell(getUnitCellFromProperties(), pointGroup->crystalSystem(),
                                             pointGroup->getCoordinateSystem());

  g_log.information() << "Constrained unit cell is: " << unitCellToStr(unitCell) << '\n';

  CompositeBraggScatterer_sptr scatterers =
      CompositeBraggScatterer::create(IsotropicAtomBraggScattererParser(getProperty("Atoms"))());

  // Create a CrystalStructure-object for use with PoldiPeakCollection
  CrystalStructure crystalStructure(unitCell, spaceGroup, scatterers);

  double dMin = getProperty("LatticeSpacingMin");
  double dMax = getDMaxValue(unitCell);

  // Create PoldiPeakCollection using given parameters, set output workspace
  PoldiPeakCollection_sptr peaks = std::make_shared<PoldiPeakCollection>(crystalStructure, dMin, dMax);

  setProperty("OutputWorkspace", peaks->asTableWorkspace());
}

} // namespace Mantid::Poldi
