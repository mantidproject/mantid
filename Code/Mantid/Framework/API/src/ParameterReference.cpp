#include "MantidAPI/ParameterReference.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid
{
namespace API
{

/// Default constructor
ParameterReference::ParameterReference() : m_function(NULL), m_index(0)
{}

/// Constructor
ParameterReference::ParameterReference(IFitFunction* fun, std::size_t index)
{
  reset(fun,index);
}

/// Return pointer to the function
IFitFunction* ParameterReference::getFunction() const
{
  return m_function;
}

/// Return parameter index in that function
std::size_t ParameterReference::getIndex() const
{
  return m_index;
}

/// Reset the reference
void ParameterReference::reset(IFitFunction* fun, size_t index)
{
  IFitFunction* fLocal = fun;
  int iLocal = static_cast<int>(index);
  CompositeFunction* cf = dynamic_cast<CompositeFunction*>(fun);
  while (cf)
  {
    int iFun = cf->functionIndex(iLocal);
    fLocal = cf->getFunction(iFun);
    iLocal = fLocal->parameterIndex(cf->parameterLocalName(iLocal));
    cf = dynamic_cast<CompositeFunction*>(fLocal);
  }

  m_function = fLocal;
  m_index = iLocal;
}

/// Set the parameter
void ParameterReference::setParameter(const double& value)
{
  m_function->setParameter(static_cast<int>(m_index),value);
}

/// Get the value of the parameter
double ParameterReference::getParameter() const
{
  return m_function->getParameter(static_cast<int>(m_index));
}

} // namespace API
} // namespace Mantid

