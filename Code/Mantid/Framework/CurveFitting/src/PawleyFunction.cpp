#include "MantidCurveFitting/PawleyFunction.h"

#include "MantidAPI/FunctionFactory.h"

#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace CurveFitting {

DECLARE_FUNCTION(PawleyParameterFunction)

using namespace API;
using namespace Geometry;

PawleyParameterFunction::PawleyParameterFunction()
    : ParamFunction(), m_crystalSystem(PointGroup::Triclinic) {}

void PawleyParameterFunction::setAttribute(const std::string &attName,
                                           const Attribute &attValue) {
  if (attName == "CrystalSystem") {
    setCrystalSystem(attValue.asString());
  }

  ParamFunction::setAttribute(attName, attValue);
}

PointGroup::CrystalSystem PawleyParameterFunction::getCrystalSystem() const {
  return m_crystalSystem;
}

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

void PawleyParameterFunction::function(const FunctionDomain &domain,
                                       FunctionValues &values) const {
  UNUSED_ARG(domain);
  UNUSED_ARG(values);
}

void PawleyParameterFunction::functionDeriv(const FunctionDomain &domain,
                                            Jacobian &jacobian) {
  UNUSED_ARG(domain)
  UNUSED_ARG(jacobian);
}

void PawleyParameterFunction::init() {
  declareAttribute("CrystalSystem", IFunction::Attribute("Triclinic"));
  declareAttribute("ProfileFunction", IFunction::Attribute("Gaussian"));

  declareParameter("a", 1.0);
  declareParameter("b", 1.0);
  declareParameter("c", 1.0);

  declareParameter("Alpha", 90.0);
  declareParameter("Beta", 90.0);
  declareParameter("Gamma", 90.0);

  declareParameter("ZeroShift", 0.0);
}

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

void PawleyFunction::setCrystalSystem(const std::string &crystalSystem) {
  m_pawleyParameterFunction->setAttributeValue("CrystalSystem", crystalSystem);
  m_compositeFunction->checkFunction();
}

void PawleyFunction::setProfileFunction(const std::string &profileFunction) {
  m_pawleyParameterFunction->setAttributeValue("ProfileFunction",
                                               profileFunction);
  m_compositeFunction->checkFunction();
}

void PawleyFunction::function1D(double *out, const double *xValues,
                                const size_t nData) const {
  UNUSED_ARG(out);
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
}

void PawleyFunction::functionDeriv1D(API::Jacobian *out, const double *xValues,
                                     const size_t nData) {
  UNUSED_ARG(out);
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
}

void PawleyFunction::addPeak() {
  m_peakProfileComposite->addFunction(
      FunctionFactory::Instance().createFunction("Gaussian"));
}

void PawleyFunction::init() {
  setDecoratedFunction("CompositeFunction");

  if (!m_compositeFunction) {
    throw std::runtime_error(
        "PawleyFunction could not construct internal CompositeFunction.");
  }
}

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
