#include "MantidSINQ/PoldiCreatePeaksFromCell.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/BoundedValidator.h"

#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidAPI/ITableWorkspace.h"

namespace Mantid
{
namespace Poldi
{

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;
using Mantid::API::ITableWorkspace;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiCreatePeaksFromCell)



//----------------------------------------------------------------------------------------------
/** Constructor
   */
PoldiCreatePeaksFromCell::PoldiCreatePeaksFromCell() :
    m_pointGroups(Geometry::getAllPointGroups()),
    m_latticeCenterings(Geometry::getAllReflectionConditions())
{
}

//----------------------------------------------------------------------------------------------
/** Destructor
   */
PoldiCreatePeaksFromCell::~PoldiCreatePeaksFromCell()
{
}

const std::string PoldiCreatePeaksFromCell::name() const
{
    return "PoldiCreatePeaksFromCell";
}


//----------------------------------------------------------------------------------------------


/// Algorithm's version for identification. @see Algorithm::version
int PoldiCreatePeaksFromCell::version() const { return 1;}

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiCreatePeaksFromCell::category() const { return "SINQ\\Poldi"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PoldiCreatePeaksFromCell::summary() const { return "Summary";}

std::map<std::string, std::string> PoldiCreatePeaksFromCell::validateInputs()
{
    std::map<std::string, std::string> errorMap;

    double dMin = getProperty("dMin");
    Property *dMaxProperty = getProperty("dMax");

    double dMaxValue = boost::lexical_cast<double>(dMaxProperty->value());
    if(!dMaxProperty->isDefault() && (dMaxValue < dMin)) {
        errorMap["dMax"] = std::string("dMax is less than dMin.");
    }

    return errorMap;
}

/// Returns a PointGroup_sptr for a given string or throws std::invalid_argument.
PointGroup_sptr PoldiCreatePeaksFromCell::getPointGroup(const std::string &pointGroupString) const
{
    for(auto it = m_pointGroups.begin(); it != m_pointGroups.end(); ++it) {
        if((*it)->getName() == pointGroupString) {
            return *it;
        }
    }

    throw std::invalid_argument("PointGroup '" + pointGroupString + "' does not exist.");
}

/// Returns a ReflectionCondition_sptr for a given lattice centering name or throws std::invalid_argument.
ReflectionCondition_sptr PoldiCreatePeaksFromCell::getLatticeCentering(const std::string &latticeCenteringString) const
{
    for(auto it = m_latticeCenterings.begin(); it != m_latticeCenterings.end(); ++it) {
        if((*it)->getName() == latticeCenteringString) {
            return *it;
        }
    }

    throw std::invalid_argument("LatticeCentering '" + latticeCenteringString + "' does not exist.");
}

/** Returns the largest lattice spacing based on the algorithm properties
 *
 *  This method returns the largest allowed lattice spacing for calculations. If the
 *  user has not supplied a value for dMax, this value is determined from the UnitCell-object.
 *  The largest possible spacing is equal to the largest cell edge. To avoid problems
 *  with floating point comparison and different use of < and <=, 1.0 is added to this value.
 *
 *  If dMax is not default, this value is used, no matter if it's larger or smaller than
 *  the maximum determined by the cell.
 *
 *  @param unitCell :: Unit cell which determines the limit
 *  @return Largest considered lattice spacing
 */
double PoldiCreatePeaksFromCell::getDMaxValue(const UnitCell &unitCell) const
{
    Property *dMaxProperty = getProperty("dMax");

    if(dMaxProperty->isDefault()) {
        // Instead of returning just the value, 1.0 is added to avoid running into problems with comparison operators
        return getLargestDValue(unitCell) + 1.0;
    }

    return getProperty("dMax");
}

/// Returns the largest possible lattice spacing for the given cell.
double PoldiCreatePeaksFromCell::getLargestDValue(const UnitCell &unitCell) const
{
    return std::max(std::max(unitCell.a(), unitCell.b()), unitCell.c());
}

/// Constructs a UnitCell-object from the algorithm properties.
UnitCell PoldiCreatePeaksFromCell::getUnitCellFromProperties() const
{
    double a = getProperty("a");
    double b = getProperty("b");
    double c = getProperty("c");

    double alpha = getProperty("alpha");
    double beta = getProperty("beta");
    double gamma = getProperty("gamma");

    return UnitCell(a, b, c, alpha, beta, gamma);
}

/** Returns a new UnitCell-object with crystal system constraints taken into account
 *
 *  This method constructs a new UnitCell-object based on the values of the supplied cell,
 *  but takes into account the constraints of the crystal system. For monoclinic, a unique b-axis is assumed.
 *
 *  It's useful for "cleaning" user input.
 *
 * @param unitCell :: UnitCell-object which should be constrained
 * @param crystalSystem :: Crystal system which is used for constraints
 * @return UnitCell-object with applied constraints
 */
UnitCell PoldiCreatePeaksFromCell::getConstrainedUnitCell(const UnitCell &unitCell, const PointGroup::CrystalSystem &crystalSystem) const
{
    switch(crystalSystem) {
    case PointGroup::Cubic:
        return UnitCell(unitCell.a(), unitCell.a(), unitCell.a());
    case PointGroup::Tetragonal:
        return UnitCell(unitCell.a(), unitCell.a(), unitCell.c());
    case PointGroup::Orthorhombic:
        return UnitCell(unitCell.a(), unitCell.b(), unitCell.c());
    case PointGroup::Monoclinic:
        return UnitCell(unitCell.a(), unitCell.b(), unitCell.c(), 90.0, unitCell.beta(), 90.0);
    case PointGroup::Hexagonal:
        return UnitCell(unitCell.a(), unitCell.a(), unitCell.c(), 90.0, 90.0, 120.0);
    case PointGroup::Trigonal:
        return UnitCell(unitCell.a(), unitCell.a(), unitCell.a(), unitCell.alpha(), unitCell.alpha(), unitCell.alpha());
    default:
        return UnitCell(unitCell);
    }
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
   */
void PoldiCreatePeaksFromCell::init()
{
    std::vector<std::string> centeringOptions;
    for (size_t i = 0; i < m_latticeCenterings.size(); ++i) {
        centeringOptions.push_back(m_latticeCenterings[i]->getName());
    }
    declareProperty("LatticeCentering", centeringOptions[0], boost::make_shared<StringListValidator>(centeringOptions),
            "Centering of the crystal lattice.");

    std::vector<std::string> pointGroupOptions;
    for (size_t i = 0; i < m_pointGroups.size(); ++i) {
        pointGroupOptions.push_back(m_pointGroups[i]->getName());
    }
    declareProperty("PointGroup", pointGroupOptions[0], boost::make_shared<StringListValidator>(pointGroupOptions),
            "Point group of the crystal.");


    boost::shared_ptr<BoundedValidator<double> > latticeParameterEdgeValidator = boost::make_shared<BoundedValidator<double> >(0.0, 0.0);
    latticeParameterEdgeValidator->clearUpper();
    declareProperty("a", 1.0, latticeParameterEdgeValidator, "Lattice parameter a");
    declareProperty("b", 1.0, latticeParameterEdgeValidator->clone(), "Lattice parameter b");
    declareProperty("c", 1.0, latticeParameterEdgeValidator->clone(), "Lattice parameter c");

    boost::shared_ptr<BoundedValidator<double> > latticeParameterAngleValidator = boost::make_shared<BoundedValidator<double> >(0.0, 180.0);
    declareProperty("alpha", 90.0, latticeParameterAngleValidator, "Lattice parameter alpha");
    declareProperty("beta", 90.0, latticeParameterAngleValidator->clone(), "Lattice parameter beta");
    declareProperty("gamma", 90.0, latticeParameterAngleValidator->clone(), "Lattice parameter gamma");

    boost::shared_ptr<BoundedValidator<double> > dValidator = boost::make_shared<BoundedValidator<double> >(0.01, 0.0);
    dValidator->clearUpper();

    declareProperty("dMin", 0.5, dValidator, "Smallest allowed lattice spacing.");
    declareProperty("dMax", 0.0, "Largest allowed lattice spacing.");

    declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace","",Direction::Output), "List with calculated peaks.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
   */
void PoldiCreatePeaksFromCell::exec()
{
    // Get all user input regarding the unit cell
    PointGroup_sptr pointGroup = getPointGroup(getProperty("PointGroup"));
    UnitCell unitCell = getConstrainedUnitCell(getUnitCellFromProperties(), pointGroup->crystalSystem());
    ReflectionCondition_sptr latticeCentering = getLatticeCentering(getProperty("LatticeCentering"));

    // Create a CrystalStructure-object for use with PoldiPeakCollection
    CrystalStructure_sptr crystalStructure = boost::make_shared<CrystalStructure>(unitCell, pointGroup, latticeCentering);

    double dMin = getProperty("dMin");
    double dMax = getDMaxValue(unitCell);

    // Create PoldiPeakCollection using given parameters, set output workspace
    PoldiPeakCollection_sptr peaks = boost::make_shared<PoldiPeakCollection>(crystalStructure, dMin, dMax);

    setProperty("OutputWorkspace", peaks->asTableWorkspace());
}



} // namespace SINQ
} // namespace Mantid
