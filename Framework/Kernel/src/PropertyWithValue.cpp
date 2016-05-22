#include "MantidKernel/PropertyWithValue.h"
#include "mantidKernel/Matrix.h"

namespace Mantid {

namespace Kernel {

/// @cond
#define INSTANTIATE(Type)                                                      \
  template DLLExport class PropertyWithValue<Type>;                            \
  template DLLExport class PropertyWithValue<std::vector<Type>>;

// Explicit instantiations
INSTANTIATE(int32_t)
INSTANTIATE(int64_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(bool)
INSTANTIATE(OptionalBool)
INSTANTIATE(double)
INSTANTIATE(std::string)
/// @endcond

} // namespace Kernel
} // namespace Mantid
