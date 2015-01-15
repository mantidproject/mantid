//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/ImmutableCompositeFunction.h"

#include <algorithm>

namespace Mantid {
namespace API {

using std::size_t;

//-----------------------------------------------------------------------------------------------
/**
 * Overridden method creates an initialization string which makes it look like a
 * siple function.
 */
std::string ImmutableCompositeFunction::asString() const {
  return IFunction::asString();
}

/**
 * A convenience method to add a new function.
 * @param fun @@ A pointer to a newly created function.
 */
void ImmutableCompositeFunction::addFunction(IFunction *fun) {
  addFunction(IFunction_sptr(fun));
}

/**
 * Set parameter by name.
 * @param name :: An alias or a name in CompositeFunction's style: f#.name
 * @param value :: A new parameter value.
 * @param explicitlySet :: The explicitly set flag.
 */
void ImmutableCompositeFunction::setParameter(const std::string &name,
                                              const double &value,
                                              bool explicitlySet) {
  auto alias = m_alias.find(name);
  if (alias != m_alias.end()) {
    CompositeFunction::setParameter(alias->second, value, explicitlySet);
  } else {
    CompositeFunction::setParameter(name, value, explicitlySet);
  }
}

/**
 * Set description of parameter by name.
 * @param name :: An alias or a name in CompositeFunction's style: f#.name
 * @param description :: A parameter description.
 */
void ImmutableCompositeFunction::setParameterDescription(
    const std::string &name, const std::string &description) {
  auto alias = m_alias.find(name);
  if (alias != m_alias.end()) {
    CompositeFunction::setParameterDescription(alias->second, description);
  } else {
    CompositeFunction::setParameterDescription(name, description);
  }
}

/**
 * Get parameter by name.
 * @param name :: An alias or a name in CompositeFunction's style: f#.name
 * @return :: The parameter value.
 */
double ImmutableCompositeFunction::getParameter(const std::string &name) const {
  auto alias = m_alias.find(name);
  if (alias != m_alias.end()) {
    return CompositeFunction::getParameter(alias->second);
  }
  return CompositeFunction::getParameter(name);
}

/**
 * Returns the index of parameter name
 * @param name :: An alias or a name in CompositeFunction's style: f#.name
 */
size_t
ImmutableCompositeFunction::parameterIndex(const std::string &name) const {
  auto alias = m_alias.find(name);
  if (alias != m_alias.end()) {
    return alias->second;
  }
  return CompositeFunction::parameterIndex(name);
}

/**
 * Returns the alias or name of parameter i
 */
std::string ImmutableCompositeFunction::parameterName(size_t i) const {
  for (auto alias = m_alias.begin(); alias != m_alias.end(); ++alias) {
    if (alias->second == i)
      return alias->first;
  }
  return CompositeFunction::parameterName(i);
}

//-----------------------------------------------------------------------------------------------
/**
 * Define an alias for a parameter. Use this method only after all member
 * functions have been added.
 * @param parName :: Fully qualified parameter name.
 * @param alias :: An alias for the parameter.
 */
void ImmutableCompositeFunction::setAlias(const std::string &parName,
                                          const std::string &alias) {
  // make sure the alias is unique
  if (m_alias.count(alias) > 0) {
    throw Kernel::Exception::ExistsError("ImmutableCompositeFunction", alias);
  }
  m_alias[alias] = CompositeFunction::parameterIndex(parName);
}

/**
 * Add default ties. Ties added with this method are created with isDefault flag
 * set to true and
 * do not appear in the string returned by asString() method.
 * @param ties :: Comma-separated list of name=value pairs where name is a
 * parameter name and value
 *  is a math expression tying the parameter to other parameters or a constant.
 */
void ImmutableCompositeFunction::addDefaultTies(const std::string &ties) {
  CompositeFunction::addTies(ties, true);
}

/**
 * Add a list of constraints from a string. Constraints added with this method
 * are created with
 * isDefault flag set to true and do not appear in the string returned by
 * asString() method.
 * @param constraints :: A comma-separated list of constraint expressions.
 */
void ImmutableCompositeFunction::addDefaultConstraints(
    const std::string &constraints) {
  CompositeFunction::addConstraints(constraints, true);
}

} // namespace API
} // namespace Mantid
