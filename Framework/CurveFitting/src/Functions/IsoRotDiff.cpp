// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/IsoRotDiff.h"
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/ElasticIsoRotDiff.h"
#include "MantidCurveFitting/Functions/InelasticIsoRotDiff.h"
// Mantid headers from other projects
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
#include "MantidAPI/IFunction.h"
// third party library headers (N/A)
#include <boost/math/special_functions/bessel.hpp>
// standard library headers (N/A)
#include <cmath>
#include <limits>

using Mantid::API::IFunction;
using Mantid::API::ImmutableCompositeFunction;
using Attribute = Mantid::API::IFunction::Attribute;

namespace {
Mantid::Kernel::Logger g_log("IsoRotDiff");
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(IsoRotDiff)

/**
 * @brief Propagate the attribute to its member functions.
 * NOTE: we pass this->getAttribute(name) by reference, thus the same
 * object is shared by the composite function and its members.
 * @param name Name of the attribute
 */
void IsoRotDiff::trickleDownAttribute(const std::string &name) {
  for (size_t iFun = 0; iFun < this->nFunctions(); iFun++) {
    auto fun = this->getFunction(iFun);
    if (fun->hasAttribute(name)) {
      fun->setAttribute(name, this->getAttribute(name));
    }
  }
}

/**
 * @brief Overwrite attributes of member functions with same name
 */
void IsoRotDiff::declareAttribute(const std::string &name,
                                  const Attribute &defaultValue) {
  ImmutableCompositeFunction::declareAttribute(name, defaultValue);
  this->trickleDownAttribute(name);
}

/**
 * @brief Overwrite attributes of member functions with same name
 * @param name Name of the attribute
 * @param att the attribute to be propagated to elastic and inelastic parts
 */
void IsoRotDiff::setAttribute(const std::string &name, const Attribute &att) {
  ImmutableCompositeFunction::setAttribute(name, att);
  this->trickleDownAttribute(name);
}

/**
 * @brief Initialize elastic and inelastic parts, aliases, attributes, and ties
 */
void IsoRotDiff::init() {
  m_elastic = boost::dynamic_pointer_cast<ElasticIsoRotDiff>(
      API::FunctionFactory::Instance().createFunction("ElasticIsoRotDiff"));
  this->addFunction(m_elastic);
  m_inelastic = boost::dynamic_pointer_cast<InelasticIsoRotDiff>(
      API::FunctionFactory::Instance().createFunction("InelasticIsoRotDiff"));
  this->addFunction(m_inelastic);

  this->declareAttribute("Q", API::IFunction::Attribute(0.3));
  this->declareAttribute("N", API::IFunction::Attribute(25));

  // Set the aliases
  this->setAlias("f1.Height", "Height");
  this->setAlias("f1.Radius", "Radius");
  this->setAlias("f1.Tau", "Tau");
  this->setAlias("f1.Centre", "Centre");

  // Set the ties between Elastic and Inelastic parameters
  this->addDefaultTies(
      "f1.Height=f0.Height,f1.Radius=f0.Radius,f1.Centre=f0.Centre");
  this->applyTies();
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
