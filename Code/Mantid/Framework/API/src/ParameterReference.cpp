#include "MantidAPI/ParameterReference.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid
{
namespace API
{

void ParameterReference::reset(IFitFunction* fun,size_t index)
{
  IFitFunction* fLocal = fun;
  size_t iLocal = index;
  CompositeFunction* cf = dynamic_cast<CompositeFunction*>(fun);
  while (cf)
  {
    size_t iFun = cf->functionIndex(iLocal);
    fLocal = cf->getFunction(iFun);
    iLocal = fLocal->parameterIndex(cf->parameterLocalName(iLocal));
    cf = dynamic_cast<CompositeFunction*>(fLocal);
  }

  m_function = fLocal;
  m_index = iLocal;
}

} // namespace API
} // namespace Mantid

