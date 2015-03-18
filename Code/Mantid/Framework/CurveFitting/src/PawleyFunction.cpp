#include "MantidCurveFitting/PawleyFunction.h"

#include "MantidAPI/FunctionFactory.h"

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace CurveFitting {

DECLARE_FUNCTION(PawleyParameterFunction)

using namespace API;
using namespace Geometry;

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
 * different number of parameters.
 *
 * @param crystalSystem :: Crystal system, case insensitive.
 */
void
PawleyParameterFunction::setCrystalSystem(const std::string &crystalSystem) {
  std::string crystalSystemLC = boost::algorithm::to_lower_copy(crystalSystem);

  if (crystalSystemLC == "cubic") {
    m_crystalSystem = PointGroup::Cubic;
  } else if (crystalSystemLC == "tetragonal") {
    m_crystalSystem = PointGroup::Tetragonal;
  } else if (crystalSystemLC == "hexagonal") {
    m_crystalSystem = PointGroup::Hexagonal;
  } else if (crystalSystemLC == "trigonal") {
    m_crystalSystem = PointGroup::Trigonal;
  } else if (crystalSystemLC == "orthorhombic") {
    m_crystalSystem = PointGroup::Orthorhombic;
  } else if (crystalSystemLC == "monoclinic") {
    m_crystalSystem = PointGroup::Monoclinic;
  } else if (crystalSystemLC == "triclinic") {
    m_crystalSystem = PointGroup::Triclinic;
  } else {
    throw std::invalid_argument("Not a valid crystal system: '" +
                                crystalSystem + "'.");
  }

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
    break;

  case PointGroup::Hexagonal:
  case PointGroup::Tetragonal:
    declareParameter("a", 1.0);
    declareParameter("c", 1.0);
    break;

  case PointGroup::Orthorhombic:
    declareParameter("a", 1.0);
    declareParameter("b", 1.0);
    declareParameter("c", 1.0);
    break;

  case PointGroup::Monoclinic:
    declareParameter("a", 1.0);
    declareParameter("b", 1.0);
    declareParameter("c", 1.0);
    declareParameter("Beta", 90.0);
    break;

  case PointGroup::Trigonal:
    declareParameter("a", 1.0);
    declareParameter("Alpha", 90.0);
    break;

  default:
    // triclinic
    declareParameter("a", 1.0);
    declareParameter("b", 1.0);
    declareParameter("c", 1.0);

    declareParameter("Alpha", 90.0);
    declareParameter("Beta", 90.0);
    declareParameter("Gamma", 90.0);
    break;
  }

  declareParameter("ZeroShift", 0.0);
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
    : FunctionParameterDecorator(), m_compositeFunction(),
      m_pawleyParameterFunction(), m_peakProfileComposite(), m_hkls() {}

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
  UnitCell cell = m_pawleyParameterFunction->getUnitCellFromParameters();
  double zeroShift = m_pawleyParameterFunction->getParameter("ZeroShift");

  for (size_t i = 0; i < m_hkls.size(); ++i) {
    double d = cell.d(m_hkls[i]) + zeroShift;

    m_peakProfileComposite->getFunction(i)->setParameter(
        m_pawleyParameterFunction->getProfileFunctionCenterParameterName(), d);
  }

  m_peakProfileComposite->function(domain, values);
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
