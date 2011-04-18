#include "MantidMDAlgorithms/NullImplicitFunction.h"
#include "MantidAPI/Point3D.h"

namespace Mantid
{
namespace MDAlgorithms
{

NullImplicitFunction::NullImplicitFunction()
{
}

NullImplicitFunction::~NullImplicitFunction()
{
}

std::string NullImplicitFunction::getName() const
{
  return NullImplicitFunction::functionName();
}

std::string NullImplicitFunction::toXMLString() const
{
  return std::string();
}

bool NullImplicitFunction::evaluate(const API::Point3D* pPoint) const
{
  UNUSED_ARG(pPoint);
  throw std::logic_error("Cannot evaluate a NullImplicitFunction. No correct behaviour for this.");
}

}
}
