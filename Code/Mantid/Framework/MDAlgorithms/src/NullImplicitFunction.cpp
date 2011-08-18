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

bool NullImplicitFunction::evaluate(const API::Point3D*) const
{
  return true; //Essentially do nothing.
}

bool NullImplicitFunction::evaluate(const Mantid::coord_t*, const bool *, const size_t) const
{
  return true; //Essentially do nothing.
}

}
}
