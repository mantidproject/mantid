#include "MantidKernel/Utils.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{

namespace Utils
{

  long round(double x)
  {
    return long(x + (x<0?-0.5:+0.5));
  }



} // namespace Utils


} // namespace Mantid
} // namespace Kernel

