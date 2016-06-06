#include "MantidKernel/PropertyWithValue.h"

namespace Mantid {

namespace Kernel {
/// @cond
#define INSTANTIATE(Type)                                                      \
  template class DLLExport PropertyWithValue<Type>;                            \
  template class DLLExport PropertyWithValue<std::vector<Type>>;

// Explicit instantiations
INSTANTIATE(int)
INSTANTIATE(long)
INSTANTIATE(long long)
INSTANTIATE(unsigned short int)
INSTANTIATE(unsigned int)
INSTANTIATE(unsigned long)
INSTANTIATE(unsigned long long)
INSTANTIATE(bool)
INSTANTIATE(OptionalBool)
INSTANTIATE(double)
INSTANTIATE(std::string)
/// @endcond

} // namespace Kernel
} // namespace Mantid
