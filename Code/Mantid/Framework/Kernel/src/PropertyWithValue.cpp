#include "MantidKernel/PropertyWithValue.h"

namespace Mantid {

namespace Kernel {
/// @cond
#define INSTANTIATE(Type)                                                      \
  template DLLExport class PropertyWithValue<Type>;                            \
  template DLLExport class PropertyWithValue<std::vector<Type>>;

// Explicit instantiations
INSTANTIATE(int)
INSTANTIATE(long)
INSTANTIATE(long long)
INSTANTIATE(unsigned short int)
INSTANTIATE(unsigned int)
INSTANTIATE(unsigned long)
INSTANTIATE(unsigned long long)
INSTANTIATE(bool)
INSTANTIATE(double)
INSTANTIATE(std::string)
/// @endcond

} // namespace Kernel
} // namespace Mantid
