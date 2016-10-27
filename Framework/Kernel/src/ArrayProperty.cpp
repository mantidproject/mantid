#include "MantidKernel/ArrayProperty.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.tcc"

namespace Mantid {
namespace Kernel {

/// @cond

template class DLLExport ArrayProperty<int32_t>;
template class DLLExport ArrayProperty<int64_t>;
template class DLLExport ArrayProperty<size_t>;
template class DLLExport ArrayProperty<double>;
template class DLLExport ArrayProperty<std::string>;

template class DLLExport ArrayProperty<std::vector<std::string>>;

/// @endcond

} // namespace Kernel
} // namespace Mantid
