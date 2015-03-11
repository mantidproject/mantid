#include "MantidCurveFitting/PawleyFunction.h"
#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace CurveFitting {

using namespace API;
using namespace Geometry;

PawleyFunction::PawleyFunction()
    : ParamFunction(), m_crystalSystem(PointGroup::Triclinic), m_unitCell() {}

void PawleyFunction::setAttribute(const std::string &attName,
                                  const Attribute &attValue) {
  if (attName == "CrystalSystem") {
    setCrystalSystem(attValue.asString());
  }

  ParamFunction::setAttribute(attName, attValue);
}

PointGroup::CrystalSystem PawleyFunction::getCrystalSystem() const {
  return m_crystalSystem;
}

void PawleyFunction::function1D(double *out, const double *xValues,
                                const size_t nData) const {
  UNUSED_ARG(out);
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
}

void PawleyFunction::functionDeriv1D(Jacobian *out, const double *xValues,
                                     const size_t nData) {
  UNUSED_ARG(out);
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
}

void PawleyFunction::functionDeriv(const FunctionDomain &domain,
                                   Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

void PawleyFunction::init() {
  declareAttribute("CrystalSystem", IFunction::Attribute("Triclinic"));
}

void PawleyFunction::setCrystalSystem(const std::string &crystalSystem) {
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
}

} // namespace CurveFitting
} // namespace Mantid
