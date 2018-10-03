// Mantid Coding standards <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/DiffRotDiscreteCircle.h"
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/ElasticDiffRotDiscreteCircle.h"
#include "MantidCurveFitting/Functions/InelasticDiffRotDiscreteCircle.h"
// Mantid headers from other projects
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
// 3rd party library headers (N/A)
// standard library headers  (N/A)

namespace {
Mantid::Kernel::Logger g_log("DiffRotDiscreteCircle");
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(DiffRotDiscreteCircle)

/**
 * @brief Propagate the attribute to its member functions.
 * NOTE: we pass this->getAttribute(name) by reference, thus the same
 * object is shared by the composite function and its members.
 */
void DiffRotDiscreteCircle::trickleDownAttribute(const std::string &name) {
  for (size_t iFun = 0; iFun < nFunctions(); iFun++) {
    API::IFunction_sptr fun = this->getFunction(iFun);
    if (fun->hasAttribute(name))
      fun->setAttribute(name, this->getAttribute(name));
  }
}

/**
 * @brief Overwrite attributes of member functions with the same name
 */
void DiffRotDiscreteCircle::declareAttribute(
    const std::string &name, const API::IFunction::Attribute &defaultValue) {
  API::ImmutableCompositeFunction::declareAttribute(name, defaultValue);
  this->trickleDownAttribute(name);
}

/**
 * @brief Overwrite attributes of member functions with the same name
 */
void DiffRotDiscreteCircle::setAttribute(const std::string &name,
                                         const Attribute &att) {
  API::ImmutableCompositeFunction::setAttribute(name, att);
  this->trickleDownAttribute(name);
}

/**
 * @brief Initialize elastic and inelastic parts, aliases, attributes, and ties
 */
void DiffRotDiscreteCircle::init() {
  m_elastic = boost::dynamic_pointer_cast<ElasticDiffRotDiscreteCircle>(
      API::FunctionFactory::Instance().createFunction(
          "ElasticDiffRotDiscreteCircle"));
  this->addFunction(m_elastic);
  m_inelastic = boost::dynamic_pointer_cast<InelasticDiffRotDiscreteCircle>(
      API::FunctionFactory::Instance().createFunction(
          "InelasticDiffRotDiscreteCircle"));
  this->addFunction(m_inelastic);

  this->setAttributeValue("NumDeriv", true);

  this->declareAttribute("Q", API::IFunction::Attribute(0.5));
  this->declareAttribute("N", API::IFunction::Attribute(3));

  // Set the aliases
  this->setAlias("f1.Intensity", "Intensity");
  this->setAlias("f1.Radius", "Radius");
  this->setAlias("f1.Decay", "Decay");
  this->setAlias("f1.Shift", "Shift");

  // Set the ties between Elastic and Inelastic parameters
  this->addDefaultTies("f0.Height=f1.Intensity,f0.Radius=f1.Radius");
  this->applyTies();
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
