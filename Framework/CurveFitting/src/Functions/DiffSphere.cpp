// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/DiffSphere.h"
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/ElasticDiffSphere.h"
#include "MantidCurveFitting/Functions/InelasticDiffSphere.h"
// Mantid headers from other projects
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
// third party library headers (N/A)
// standard library headers (N/A)

#include <boost/math/special_functions/bessel.hpp>

#include <cmath>
#include <limits>

namespace {
Mantid::Kernel::Logger g_log("DiffSphere");
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(DiffSphere)

/**
 * @brief Propagate the attribute to its member functions.
 * NOTE: we pass this->getAttribute(name) by reference, thus the same
 * object is shared by the composite function and its members.
 * @param name Name of the attribute
 */
void DiffSphere::trickleDownAttribute(const std::string &name) {
  for (size_t iFun = 0; iFun < this->nFunctions(); iFun++) {
    API::IFunction_sptr fun = this->getFunction(iFun);
    if (fun->hasAttribute(name)) {
      fun->setAttribute(name, this->getAttribute(name));
    }
  }
}

/**
 * @brief Overwrite attributes of member functions with same name
 */
void DiffSphere::declareAttribute(
    const std::string &name, const API::IFunction::Attribute &defaultValue) {
  API::ImmutableCompositeFunction::declareAttribute(name, defaultValue);
  this->trickleDownAttribute(name);
}

/**
 * @brief Overwrite attributes of member functions with same name
 * @param name Name of the attribute
 * @param att the attribute to be propagated to elastic and inelastic parts
 */
void DiffSphere::setAttribute(const std::string &name,
                              const API::IFunction::Attribute &att) {
  API::ImmutableCompositeFunction::setAttribute(name, att);
  this->trickleDownAttribute(name);
}

/**
 * @brief Initialize elastic and inelastic parts, aliases, attributes, and ties
 */
void DiffSphere::init() {
  m_elastic = boost::dynamic_pointer_cast<ElasticDiffSphere>(
      API::FunctionFactory::Instance().createFunction("ElasticDiffSphere"));
  this->addFunction(m_elastic);
  m_inelastic = boost::dynamic_pointer_cast<InelasticDiffSphere>(
      API::FunctionFactory::Instance().createFunction("InelasticDiffSphere"));
  this->addFunction(m_inelastic);

  this->setAttributeValue("NumDeriv", true);
  this->declareAttribute("Q", API::IFunction::Attribute(1.0));

  // Set the aliases
  this->setAlias("f1.Intensity", "Intensity");
  this->setAlias("f1.Radius", "Radius");
  this->setAlias("f1.Diffusion", "Diffusion");
  this->setAlias("f1.Shift", "Shift");

  // Set the ties between Elastic and Inelastic parameters
  this->addDefaultTies(
      "f0.Height=f1.Intensity,f0.Radius=f1.Radius,f0.Centre=0");
  this->applyTies();
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
