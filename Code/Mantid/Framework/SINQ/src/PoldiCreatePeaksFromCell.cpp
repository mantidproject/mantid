#include "MantidSINQ/PoldiCreatePeaksFromCell.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/BoundedValidator.h"

#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/BraggScattererFactory.h"

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>

namespace Mantid {
namespace Poldi {

using Kernel::Direction;
using API::WorkspaceProperty;
using API::ITableWorkspace;
using namespace Geometry;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiCreatePeaksFromCell)

//----------------------------------------------------------------------------------------------
/** Constructor
   */
PoldiCreatePeaksFromCell::PoldiCreatePeaksFromCell() {}

//----------------------------------------------------------------------------------------------
/** Destructor
   */
PoldiCreatePeaksFromCell::~PoldiCreatePeaksFromCell() {}

const std::string PoldiCreatePeaksFromCell::name() const {
  return "PoldiCreatePeaksFromCell";
}

//----------------------------------------------------------------------------------------------

/// Algorithm's version for identification. @see Algorithm::version
int PoldiCreatePeaksFromCell::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiCreatePeaksFromCell::category() const {
  return "SINQ\\Poldi";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PoldiCreatePeaksFromCell::summary() const {
  return "Generate a TableWorkspace with all symmetry independent reflections "
         "using a unit cell.";
}

std::map<std::string, std::string> PoldiCreatePeaksFromCell::validateInputs() {
  std::map<std::string, std::string> errorMap;

  double dMin = getProperty("LatticeSpacingMin");
  Property *dMaxProperty = getProperty("LatticeSpacingMax");

  double dMax = boost::lexical_cast<double>(dMaxProperty->value());
  if (!dMaxProperty->isDefault() && (dMax < dMin)) {
    errorMap["LatticeSpacingMax"] =
        std::string("LatticeSpacingMax is less than LatticeSpacingMin.");
  }

  return errorMap;
}

/// Tries to construct a space group object using the space group factory.
SpaceGroup_const_sptr PoldiCreatePeaksFromCell::getSpaceGroup(
    const std::string &spaceGroupString) const {
  return SpaceGroupFactory::Instance().createSpaceGroup(spaceGroupString);
}

CompositeBraggScatterer_sptr PoldiCreatePeaksFromCell::getScatterers(
    const std::string &scattererString) const {
  boost::char_separator<char> atomSep(";");
  boost::tokenizer<boost::char_separator<char>> tokens(scattererString,
                                                       atomSep);

  std::vector<BraggScatterer_sptr> scatterers;

  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    scatterers.push_back(getScatterer(boost::trim_copy(*it)));
  }

  return CompositeBraggScatterer::create(scatterers);
}

BraggScatterer_sptr PoldiCreatePeaksFromCell::getScatterer(
    const std::string &singleScatterer) const {
  std::vector<std::string> tokens;
  boost::split(tokens, singleScatterer, boost::is_any_of(" "));

  if (tokens.size() < 4 || tokens.size() > 6) {
    throw std::invalid_argument("Could not parse scatterer string: " +
                                singleScatterer);
  }

  std::vector<std::string> cleanScattererTokens =
      getCleanScattererTokens(tokens);
  std::vector<std::string> properties =
      boost::assign::list_of("Element")("Position")("Occupancy")("U")
          .convert_to_container<std::vector<std::string>>();

  std::string initString;
  for (size_t i = 0; i < cleanScattererTokens.size(); ++i) {
    initString += properties[i] + "=" + cleanScattererTokens[i] + ";";
  }

  return BraggScattererFactory::Instance().createScatterer(
      "IsotropicAtomBraggScatterer", initString);
}

std::vector<std::string> PoldiCreatePeaksFromCell::getCleanScattererTokens(
    const std::vector<std::string> &tokens) const {
  std::vector<std::string> cleanTokens;

  // Element
  cleanTokens.push_back(tokens[0]);

  // X, Y, Z
  cleanTokens.push_back("[" + tokens[1] + "," + tokens[2] + "," + tokens[3] +
                        "]");

  for (size_t i = 4; i < tokens.size(); ++i) {
    cleanTokens.push_back(tokens[i]);
  }

  return cleanTokens;
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
  Property *dMaxProperty = getProperty("LatticeSpacingMax");

  if (dMaxProperty->isDefault()) {
    // Instead of returning just the value, 1.0 is added to avoid running into
    // problems with comparison operators
    return getLargestDValue(unitCell) + 1.0;
  }

  return getProperty("LatticeSpacingMax");
}

/// Returns the largest possible lattice spacing for the given cell.
double
PoldiCreatePeaksFromCell::getLargestDValue(const UnitCell &unitCell) const {
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
 *account
 *
 *  This method constructs a new UnitCell-object based on the values of the
 *supplied cell,
 *  but takes into account the constraints of the crystal system. For
 *monoclinic, a unique b-axis is assumed.
 *
 *  It's useful for "cleaning" user input.
 *
 * @param unitCell :: UnitCell-object which should be constrained
 * @param crystalSystem :: Crystal system which is used for constraints
 * @return UnitCell-object with applied constraints
 */
UnitCell PoldiCreatePeaksFromCell::getConstrainedUnitCell(
    const UnitCell &unitCell,
    const PointGroup::CrystalSystem &crystalSystem) const {
  switch (crystalSystem) {
  case PointGroup::Cubic:
    return UnitCell(unitCell.a(), unitCell.a(), unitCell.a());
  case PointGroup::Tetragonal:
    return UnitCell(unitCell.a(), unitCell.a(), unitCell.c());
  case PointGroup::Orthorhombic:
    return UnitCell(unitCell.a(), unitCell.b(), unitCell.c());
  case PointGroup::Monoclinic:
    return UnitCell(unitCell.a(), unitCell.b(), unitCell.c(), 90.0,
                    unitCell.beta(), 90.0);
  case PointGroup::Hexagonal:
    return UnitCell(unitCell.a(), unitCell.a(), unitCell.c(), 90.0, 90.0,
                    120.0);
  case PointGroup::Trigonal:
    return UnitCell(unitCell.a(), unitCell.a(), unitCell.a(), unitCell.alpha(),
                    unitCell.alpha(), unitCell.alpha());
  default:
    return UnitCell(unitCell);
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
   */
void PoldiCreatePeaksFromCell::init() {
  std::vector<std::string> spaceGroups =
      SpaceGroupFactory::Instance().subscribedSpaceGroupSymbols();
  declareProperty("SpaceGroup", spaceGroups.front(),
                  boost::make_shared<StringListValidator>(spaceGroups),
                  "SpaceGroup of the crystal structure.");

  declareProperty("Atoms", "", "Atoms in the asymmetric unit. Format: \n"
                               "Element x y z Occupancy U; ... ");

  boost::shared_ptr<BoundedValidator<double>> latticeParameterEdgeValidator =
      boost::make_shared<BoundedValidator<double>>(0.0, 0.0);
  latticeParameterEdgeValidator->clearUpper();
  declareProperty("a", 1.0, latticeParameterEdgeValidator,
                  "Lattice parameter a");
  declareProperty("b", 1.0, latticeParameterEdgeValidator->clone(),
                  "Lattice parameter b");
  declareProperty("c", 1.0, latticeParameterEdgeValidator->clone(),
                  "Lattice parameter c");

  boost::shared_ptr<BoundedValidator<double>> latticeParameterAngleValidator =
      boost::make_shared<BoundedValidator<double>>(0.0, 180.0);
  declareProperty("alpha", 90.0, latticeParameterAngleValidator,
                  "Lattice parameter alpha");
  declareProperty("beta", 90.0, latticeParameterAngleValidator->clone(),
                  "Lattice parameter beta");
  declareProperty("gamma", 90.0, latticeParameterAngleValidator->clone(),
                  "Lattice parameter gamma");

  boost::shared_ptr<BoundedValidator<double>> dValidator =
      boost::make_shared<BoundedValidator<double>>(0.01, 0.0);
  dValidator->clearUpper();

  declareProperty("LatticeSpacingMin", 0.5, dValidator,
                  "Smallest allowed lattice spacing.");
  declareProperty("LatticeSpacingMax", 0.0, "Largest allowed lattice spacing.");

  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "List with calculated peaks.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
   */
void PoldiCreatePeaksFromCell::exec() {
  // Get all user input regarding the unit cell
  SpaceGroup_const_sptr spaceGroup = getSpaceGroup(getProperty("SpaceGroup"));
  PointGroup_sptr pointGroup =
      PointGroupFactory::Instance().createPointGroupFromSpaceGroupSymbol(
          spaceGroup->hmSymbol());
  UnitCell unitCell = getConstrainedUnitCell(getUnitCellFromProperties(),
                                             pointGroup->crystalSystem());
  CompositeBraggScatterer_sptr scatterers = getScatterers(getProperty("Atoms"));

  // Create a CrystalStructure-object for use with PoldiPeakCollection
  CrystalStructure_sptr crystalStructure =
      boost::make_shared<CrystalStructure>(unitCell, spaceGroup, scatterers);

  double dMin = getProperty("LatticeSpacingMin");
  double dMax = getDMaxValue(unitCell);

  // Create PoldiPeakCollection using given parameters, set output workspace
  PoldiPeakCollection_sptr peaks =
      boost::make_shared<PoldiPeakCollection>(crystalStructure, dMin, dMax);

  setProperty("OutputWorkspace", peaks->asTableWorkspace());
}

} // namespace SINQ
} // namespace Mantid
