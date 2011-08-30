#include "MantidKernel/VMD.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Kernel
{

/**
  Prints a text representation of itself
  @param os :: the Stream to output to
  @param v :: the vector to output
  @return the output stream
  */
std::ostream& operator<<(std::ostream& os, const VMD& v)
{
  os << v.toString();
  return os;
}


} // namespace Mantid
} // namespace Kernel

