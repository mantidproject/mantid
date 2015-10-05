//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Kernel {

/// @cond

template DLLExport class ArrayProperty<int32_t>;
template DLLExport class ArrayProperty<int64_t>;
template DLLExport class ArrayProperty<size_t>;
template DLLExport class ArrayProperty<double>;
template DLLExport class ArrayProperty<std::string>;

template DLLExport class ArrayProperty<std::vector<std::string>>;

/// @endcond

} // namespace Kernel
} // namespace Mantid
