//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Resolution.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Resolution)

Resolution::Resolution() : ParamFunction(), IFunction1D() {}

void Resolution::function1D(double *out, const double *xValues,
                            const size_t nData) const {
  m_fun.function1D(out, xValues, nData);
}

void Resolution::functionDeriv1D(Jacobian *, const double *, const size_t) {
  // do nothing: no fitting parameters
}

size_t Resolution::nAttributes() const { return m_fun.nAttributes(); }

std::vector<std::string> Resolution::getAttributeNames() const {
  return m_fun.getAttributeNames();
}

IFunction::Attribute
Resolution::getAttribute(const std::string &attName) const {
  return m_fun.getAttribute(attName);
}

void Resolution::setAttribute(const std::string &attName,
                              const IFunction::Attribute &value) {
  m_fun.setAttribute(attName, value);
}

bool Resolution::hasAttribute(const std::string &attName) const {
  return m_fun.hasAttribute(attName);
}

} // namespace CurveFitting
} // namespace Mantid
