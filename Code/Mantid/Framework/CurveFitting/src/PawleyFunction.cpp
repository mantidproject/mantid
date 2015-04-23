#include "MantidCurveFitting/PawleyFunction.h"

#include "MantidAPI/FunctionFactory.h"

#include "MantidCurveFitting/BoundaryConstraint.h"

#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace CurveFitting {

DECLARE_FUNCTION(PawleyParameterFunction)

using namespace API;
using namespace Geometry;
using namespace Kernel;

/// Constructor
PawleyParameterFunction::PawleyParameterFunction()
    : ParamFunction(), m_crystalSystem(PointGroup::Triclinic),
      m_profileFunctionCenterParameterName() {}

/**
 * @brief Sets the supplied attribute value
 *
 * The function calls ParamFunction::setAttribute, but performs additional
 * actions for CrystalSystem and ProfileFunction.
 *
 * @param attName :: Name of the attribute
 * @param attValue :: Value of the attribute
 */
void PawleyParameterFunction::setAttribute(const std::string &attName,
                                           const Attribute &attValue) {
  if (attName == "CrystalSystem") {
    setCrystalSystem(attValue.asString());
  } else if (attName == "ProfileFunction") {
    setProfileFunction(attValue.asString());
  }

  ParamFunction::setAttribute(attName, attValue);
}

/// Returns the crystal system
PointGroup::CrystalSystem PawleyParameterFunction::getCrystalSystem() const {
  return m_crystalSystem;
}

/// Returns a UnitCell object constructed from the function's parameters.
UnitCell PawleyParameterFunction::getUnitCellFromParameters() const {
  switch (m_crystalSystem) {
  case PointGroup::Cubic: {
    double a = getParameter("a");
    return UnitCell(a, a, a);
  }
  case PointGroup::Tetragonal: {
    double a = getParameter("a");
    return UnitCell(a, a, getParameter("c"));
  }
  case PointGroup::Hexagonal: {
    double a = getParameter("a");
    return UnitCell(a, a, getParameter("c"), 90, 90, 120);
  }
  case PointGroup::Trigonal: {
    double a = getParameter("a");
    double alpha = getParameter("Alpha");
    return UnitCell(a, a, a, alpha, alpha, alpha);
  }
  case PointGroup::Orthorhombic: {
    return UnitCell(getParameter("a"), getParameter("b"), getParameter("c"));
  }
  case PointGroup::Monoclinic: {
    return UnitCell(getParameter("a"), getParameter("b"), getParameter("c"), 90,
                    getParameter("Beta"), 90);
  }
  case PointGroup::Triclinic: {
    return UnitCell(getParameter("a"), getParameter("b"), getParameter("c"),
                    getParameter("Alpha"), getParameter("Beta"),
                    getParameter("Gamma"));
  }
  }

  return UnitCell();
}

/// Sets the function's parameters from the supplied UnitCell.
void PawleyParameterFunction::setParametersFromUnitCell(const UnitCell &cell) {
  // Parameter "a" exists in all crystal systems.
  setParameter("a", cell.a());

  try {
    setParameter("b", cell.b());
  }
  catch (std::invalid_argument) {
    // do nothing.
  }

  try {
    setParameter("c", cell.c());
  }
  catch (std::invalid_argument) {
    // do nothing
  }

  try {
    setParameter("Alpha", cell.alpha());
  }
  catch (std::invalid_argument) {
    // do nothing.
  }
  try {
    setParameter("Beta", cell.beta());
  }
  catch (std::invalid_argument) {
    // do nothing.
  }
  try {
    setParameter("Gamma", cell.gamma());
  }
  catch (std::invalid_argument) {
    // do nothing.
  }
}

/// This method does nothing.
void PawleyParameterFunction::function(const FunctionDomain &domain,
                                       FunctionValues &values) const {
  UNUSED_ARG(domain);
  UNUSED_ARG(values);
}

/// This method does nothing.
void PawleyParameterFunction::functionDeriv(const FunctionDomain &domain,
                                            Jacobian &jacobian) {
  UNUSED_ARG(domain)
  UNUSED_ARG(jacobian);
}

/// Declares attributes and generates parameters based on the defaults.
void PawleyParameterFunction::init() {
  declareAttribute("CrystalSystem", IFunction::Attribute("Triclinic"));
  declareAttribute("ProfileFunction", IFunction::Attribute("Gaussian"));

  setCrystalSystem("Triclinic");
  setProfileFunction("Gaussian");
}

/**
 * Sets the profile function
 *
 * This method takes a function name and tries to create the corresponding
 * function through FunctionFactory. Then it checks whether the function
 * inherits from IPeakFunction and determines the centre parameter to store it.
 *
 * @param profileFunction :: Name of an IPeakFunction implementation.
 */
void PawleyParameterFunction::setProfileFunction(
    const std::string &profileFunction) {
  IPeakFunction_sptr peakFunction = boost::dynamic_pointer_cast<IPeakFunction>(
      FunctionFactory::Instance().createFunction(profileFunction));

  if (!peakFunction) {
    throw std::invalid_argument("PawleyFunction can only use IPeakFunctions to "
                                "calculate peak profiles.");
  }

  setCenterParameterNameFromFunction(peakFunction);
}

/**
 * Assigns the crystal system
 *
 * This method takes the name of a crystal system (case insensitive) and stores
 * it. Furthermore it creates the necessary parameters, which means that after
 * calling this function, PawleyParameterFunction potentially exposes a
 * different number of parameters. The parameters are constrained to physically
 * meaningful values (angles between 0 and 180 degrees, cell edges above 0).
 *
 * @param crystalSystem :: Crystal system, case insensitive.
 */
void
PawleyParameterFunction::setCrystalSystem(const std::string &crystalSystem) {
  m_crystalSystem = Geometry::getCrystalSystemFromString(crystalSystem);

  createCrystalSystemParameters(m_crystalSystem);
}

/// This method clears all parameters and declares parameters according to the
/// supplied crystal system.
void PawleyParameterFunction::createCrystalSystemParameters(
    PointGroup::CrystalSystem crystalSystem) {

  clearAllParameters();
  switch (crystalSystem) {
  case PointGroup::Cubic:
    declareParameter("a", 1.0);
    addLengthConstraint("a");
    break;

  case PointGroup::Hexagonal:
  case PointGroup::Tetragonal:
    declareParameter("a", 1.0);
    declareParameter("c", 1.0);
    addLengthConstraint("a");
    addLengthConstraint("c");
    break;

  case PointGroup::Orthorhombic:
    declareParameter("a", 1.0);
    declareParameter("b", 1.0);
    declareParameter("c", 1.0);
    addLengthConstraint("a");
    addLengthConstraint("b");
    addLengthConstraint("c");
    break;

  case PointGroup::Monoclinic:
    declareParameter("a", 1.0);
    declareParameter("b", 1.0);
    declareParameter("c", 1.0);
    addLengthConstraint("a");
    addLengthConstraint("b");
    addLengthConstraint("c");

    declareParameter("Beta", 90.0);
    addAngleConstraint("Beta");
    break;

  case PointGroup::Trigonal:
    declareParameter("a", 1.0);
    declareParameter("Alpha", 90.0);
    addLengthConstraint("a");
    addAngleConstraint("Alpha");
    break;

  default:
    // triclinic
    declareParameter("a", 1.0);
    declareParameter("b", 1.0);
    declareParameter("c", 1.0);
    addLengthConstraint("a");
    addLengthConstraint("b");
    addLengthConstraint("c");

    declareParameter("Alpha", 90.0);
    declareParameter("Beta", 90.0);
    declareParameter("Gamma", 90.0);
    addAngleConstraint("Alpha");
    addAngleConstraint("Beta");
    addAngleConstraint("Gamma");
    break;
  }

  declareParameter("ZeroShift", 0.0);
}

/// Adds a default constraint so that cell edge lengths can not be less than 0.
void
PawleyParameterFunction::addLengthConstraint(const std::string &parameterName) {
  BoundaryConstraint *cellEdgeConstraint =
      new BoundaryConstraint(this, parameterName, 0.0, true);
  cellEdgeConstraint->setPenaltyFactor(1e12);
  addConstraint(cellEdgeConstraint);
}

/// Adds a default constraint so cell angles are in the range 0 to 180.
void
PawleyParameterFunction::addAngleConstraint(const std::string &parameterName) {
  BoundaryConstraint *cellAngleConstraint =
      new BoundaryConstraint(this, parameterName, 0.0, 180.0, true);
  cellAngleConstraint->setPenaltyFactor(1e12);
  addConstraint(cellAngleConstraint);
}

/// Tries to extract and store the center parameter name from the function.
void PawleyParameterFunction::setCenterParameterNameFromFunction(
    const IPeakFunction_sptr &profileFunction) {
  m_profileFunctionCenterParameterName.clear();
  if (profileFunction) {
    m_profileFunctionCenterParameterName =
        profileFunction->getCentreParameterName();
  }
}

DECLARE_FUNCTION(PawleyFunction)

/// Constructor
PawleyFunction::PawleyFunction()
    : IPawleyFunction(), m_compositeFunction(), m_pawleyParameterFunction(),
      m_peakProfileComposite(), m_hkls(), m_dUnit(), m_wsUnit(),
      m_peakRadius(5) {
  int peakRadius;
  if (Kernel::ConfigService::Instance().getValue("curvefitting.peakRadius",
                                                 peakRadius)) {
    m_peakRadius = peakRadius;
  }
}

void PawleyFunction::setMatrixWorkspace(
    boost::shared_ptr<const MatrixWorkspace> workspace, size_t wi,
    double startX, double endX) {
  if (workspace) {
    Axis *xAxis = workspace->getAxis(wi);
    Kernel::Unit_sptr wsUnit = xAxis->unit();

    if (boost::dynamic_pointer_cast<Units::Empty>(wsUnit) ||
        boost::dynamic_pointer_cast<Units::dSpacing>(wsUnit)) {
      m_wsUnit = m_dUnit;
    } else {
      double factor, power;
      if (wsUnit->quickConversion(*m_dUnit, factor, power)) {
        m_wsUnit = wsUnit;
      } else {
        throw std::invalid_argument("Can not use quick conversion for unit.");
      }
    }
  }

  m_wrappedFunction->setMatrixWorkspace(workspace, wi, startX, endX);
}

/// Sets the crystal system on the internal parameter function and updates the
/// exposed parameters
void PawleyFunction::setCrystalSystem(const std::string &crystalSystem) {
  m_pawleyParameterFunction->setAttributeValue("CrystalSystem", crystalSystem);
  m_compositeFunction->checkFunction();
}

/// Sets the profile function and replaces already existing functions in the
/// internally stored CompositeFunction.
void PawleyFunction::setProfileFunction(const std::string &profileFunction) {
  m_pawleyParameterFunction->setAttributeValue("ProfileFunction",
                                               profileFunction);

  /* At this point PawleyParameterFunction guarantees that it's an IPeakFunction
   * and all existing profile functions are replaced.
   */
  for (size_t i = 0; i < m_peakProfileComposite->nFunctions(); ++i) {
    IPeakFunction_sptr oldFunction = boost::dynamic_pointer_cast<IPeakFunction>(
        m_peakProfileComposite->getFunction(i));

    IPeakFunction_sptr newFunction = boost::dynamic_pointer_cast<IPeakFunction>(
        FunctionFactory::Instance().createFunction(
            m_pawleyParameterFunction->getProfileFunctionName()));

    newFunction->setCentre(oldFunction->centre());
    try {
      newFunction->setFwhm(oldFunction->fwhm());
    }
    catch (...) {
      // do nothing.
    }
    newFunction->setHeight(oldFunction->height());

    m_peakProfileComposite->replaceFunction(i, newFunction);
  }

  // Update exposed parameters.
  m_compositeFunction->checkFunction();
}

/// Sets the unit cell from a string with either 6 or 3 space-separated numbers.
void PawleyFunction::setUnitCell(const std::string &unitCellString) {
  m_pawleyParameterFunction->setParametersFromUnitCell(
      strToUnitCell(unitCellString));
}

/// Transform d value to workspace unit
double PawleyFunction::getTransformedCenter(double d) const {
  if ((m_dUnit && m_wsUnit) && m_dUnit != m_wsUnit) {
    return UnitConversion::run(*m_dUnit, *m_wsUnit, d, 0, 0, 0,
                               DeltaEMode::Elastic, 0);
  }

  return d;
}

void PawleyFunction::setPeakPositions(std::string centreName, double zeroShift,
                                      const UnitCell &cell) const {
  for (size_t i = 0; i < m_hkls.size(); ++i) {
    double centre = getTransformedCenter(cell.d(m_hkls[i]));

    m_peakProfileComposite->getFunction(i)
        ->setParameter(centreName, centre + zeroShift);
  }
}

size_t PawleyFunction::calculateFunctionValues(
    const API::IPeakFunction_sptr &peak, const API::FunctionDomain1D &domain,
    API::FunctionValues &localValues) const {
  size_t domainSize = domain.size();
  const double *domainBegin = domain.getPointerAt(0);
  const double *domainEnd = domain.getPointerAt(domainSize);

  double centre = peak->centre();
  double dx = m_peakRadius * peak->fwhm();

  auto lb = std::lower_bound(domainBegin, domainEnd, centre - dx);
  auto ub = std::upper_bound(lb, domainEnd, centre + dx);

  size_t n = std::distance(lb, ub);

  if (n == 0) {
    throw std::invalid_argument("Null-domain");
  }

  FunctionDomain1DView localDomain(lb, n);
  localValues.reset(localDomain);

  peak->functionLocal(localValues.getPointerToCalculated(0),
                      localDomain.getPointerAt(0), n);

  return std::distance(domainBegin, lb);
}

/**
 * Calculates the function values on the supplied domain
 *
 * This function is the core of PawleyFunction. It calculates the d-value for
 * each stored HKL from the unit cell that is the result of the parameters
 * stored in the internal PawleyParameterFunction and adds the ZeroShift
 * parameter. The value is set as center parameter on the internally stored
 * PeakFunctions.
 *
 * @param domain :: Function domain.
 * @param values :: Function values.
 */
void PawleyFunction::function(const FunctionDomain &domain,
                              FunctionValues &values) const {
  values.zeroCalculated();
  try {
    const FunctionDomain1D &domain1D =
        dynamic_cast<const FunctionDomain1D &>(domain);

    UnitCell cell = m_pawleyParameterFunction->getUnitCellFromParameters();
    double zeroShift = m_pawleyParameterFunction->getParameter("ZeroShift");
    std::string centreName =
        m_pawleyParameterFunction->getProfileFunctionCenterParameterName();

    setPeakPositions(centreName, zeroShift, cell);

    FunctionValues localValues;

    for (size_t i = 0; i < m_peakProfileComposite->nFunctions(); ++i) {
      IPeakFunction_sptr peak = boost::dynamic_pointer_cast<IPeakFunction>(
          m_peakProfileComposite->getFunction(i));

      try {
        size_t offset = calculateFunctionValues(peak, domain1D, localValues);
        values.addToCalculated(offset, localValues);
      }
      catch (std::invalid_argument) {
        // do nothing
      }
    }

    setPeakPositions(centreName, 0.0, cell);
  }
  catch (std::bad_cast) {
    // do nothing
  }
}

/// Removes all peaks from the function.
void PawleyFunction::clearPeaks() {
  m_peakProfileComposite = boost::dynamic_pointer_cast<CompositeFunction>(
      FunctionFactory::Instance().createFunction("CompositeFunction"));
  m_compositeFunction->replaceFunction(1, m_peakProfileComposite);
  m_hkls.clear();
}

/// Clears peaks and adds a peak for each hkl, all with the same FWHM and
/// height.
void PawleyFunction::setPeaks(const std::vector<Kernel::V3D> &hkls, double fwhm,
                              double height) {
  clearPeaks();

  for (size_t i = 0; i < hkls.size(); ++i) {
    addPeak(hkls[i], fwhm, height);
  }
}

/// Adds a peak with the supplied FWHM and height.
void PawleyFunction::addPeak(const Kernel::V3D &hkl, double fwhm,
                             double height) {
  m_hkls.push_back(hkl);

  IPeakFunction_sptr peak = boost::dynamic_pointer_cast<IPeakFunction>(
      FunctionFactory::Instance().createFunction(
          m_pawleyParameterFunction->getProfileFunctionName()));

  peak->fix(peak->parameterIndex(
      m_pawleyParameterFunction->getProfileFunctionCenterParameterName()));

  try {
    peak->setFwhm(fwhm);
  }
  catch (...) {
    // do nothing.
  }

  peak->setHeight(height);

  m_peakProfileComposite->addFunction(peak);

  m_compositeFunction->checkFunction();
}

/// Returns the number of peaks that are stored in the function.
size_t PawleyFunction::getPeakCount() const { return m_hkls.size(); }

IPeakFunction_sptr PawleyFunction::getPeakFunction(size_t i) const {
  if (i >= m_hkls.size()) {
    throw std::out_of_range("Peak index out of range.");
  }

  return boost::dynamic_pointer_cast<IPeakFunction>(
      m_peakProfileComposite->getFunction(i));
}

/// Return the HKL of the i-th peak.
Kernel::V3D PawleyFunction::getPeakHKL(size_t i) const {
  if (i >= m_hkls.size()) {
    throw std::out_of_range("Peak index out of range.");
  }

  return m_hkls[i];
}

/// Returns the internally stored PawleyParameterFunction.
PawleyParameterFunction_sptr
PawleyFunction::getPawleyParameterFunction() const {
  return m_pawleyParameterFunction;
}

void PawleyFunction::init() {
  setDecoratedFunction("CompositeFunction");

  if (!m_compositeFunction) {
    throw std::runtime_error(
        "PawleyFunction could not construct internal CompositeFunction.");
  }

  m_dUnit = UnitFactory::Instance().create("dSpacing");
}

/// Checks that the decorated function has the correct structure.
void PawleyFunction::beforeDecoratedFunctionSet(const API::IFunction_sptr &fn) {
  CompositeFunction_sptr composite =
      boost::dynamic_pointer_cast<CompositeFunction>(fn);

  if (!composite) {
    throw std::invalid_argument("PawleyFunction only works with "
                                "CompositeFunction. Selecting another "
                                "decorated function is not possible.");
  }

  m_compositeFunction = composite;

  if (m_compositeFunction->nFunctions() == 0) {
    m_peakProfileComposite = boost::dynamic_pointer_cast<CompositeFunction>(
        FunctionFactory::Instance().createFunction("CompositeFunction"));

    m_pawleyParameterFunction =
        boost::dynamic_pointer_cast<PawleyParameterFunction>(
            FunctionFactory::Instance().createFunction(
                "PawleyParameterFunction"));

    m_compositeFunction->addFunction(m_pawleyParameterFunction);
    m_compositeFunction->addFunction(m_peakProfileComposite);
  } else {
    m_pawleyParameterFunction =
        boost::dynamic_pointer_cast<PawleyParameterFunction>(
            m_compositeFunction->getFunction(0));
    m_peakProfileComposite = boost::dynamic_pointer_cast<CompositeFunction>(
        m_compositeFunction->getFunction(1));
  }
}

} // namespace CurveFitting
} // namespace Mantid
