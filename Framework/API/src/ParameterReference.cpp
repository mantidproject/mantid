#include "MantidAPI/ParameterReference.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid {
namespace API {

/// Default constructor
ParameterReference::ParameterReference()
    : m_function(), m_index(0), m_isDefault(false) {}

/**
 * Constructor.
 * @param fun :: Pointer to a function (composite or simple).
 * @param index :: Index of a parameter of fun
 * @param isDefault :: Flag to mark as default the value of an object associated
 * with this reference:
 *  a tie or a constraint.
 */
ParameterReference::ParameterReference(IFunction *fun, std::size_t index,
                                       bool isDefault) {
  reset(fun, index, isDefault);
}

/// Return pointer to the function
IFunction *ParameterReference::getFunction() const { return m_function; }

/// Return parameter index in that function
std::size_t ParameterReference::getIndex() const { return m_index; }

/**
 * Reset the reference
 * @param fun :: Pointer to a function (composite or simple).
 * @param index :: Index of a parameter of fun
 * @param isDefault :: Flag to mark as default the value of an object associated
 * with this reference:
 *  a tie or a constraint.
 */
void ParameterReference::reset(IFunction *fun, std::size_t index,
                               bool isDefault) {
  IFunction *fLocal = fun;
  size_t iLocal = index;
  CompositeFunction *cf = dynamic_cast<CompositeFunction *>(fun);
  while (cf) {
    size_t iFun =
        cf->functionIndex(iLocal); // TODO squashing the warning breaks the code
    fLocal = cf->getFunction(iFun).get();
    iLocal = fLocal->parameterIndex(cf->parameterLocalName(iLocal));
    cf = dynamic_cast<CompositeFunction *>(fLocal);
  }

  m_function = fLocal;
  m_index = iLocal;
  m_isDefault = isDefault;
}

/**
 * Set the parameter
 * @param value :: A value to set.
 */
void ParameterReference::setParameter(const double &value) {
  m_function->setParameter(m_index, value);
}

/// Get the value of the parameter
double ParameterReference::getParameter() const {
  return m_function->getParameter(m_index);
}

/// Returns the default value flag
bool ParameterReference::isDefault() const { return m_isDefault; }

} // namespace API
} // namespace Mantid
