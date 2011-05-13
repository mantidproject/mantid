#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{

/// @cond

template DLLExport class PropertyWithValue<int32_t>;
template DLLExport class PropertyWithValue<int64_t>;
template DLLExport class PropertyWithValue<size_t>;
template DLLExport class PropertyWithValue<bool>;
template DLLExport class PropertyWithValue<double>;
template DLLExport class PropertyWithValue<std::string>;

template DLLExport class PropertyWithValue<std::vector<double> >;
template DLLExport class PropertyWithValue<std::vector<std::string> >;
template DLLExport class PropertyWithValue<std::vector<uint16_t> >;
template DLLExport class PropertyWithValue<std::vector<int16_t> >;
template DLLExport class PropertyWithValue<std::vector<uint32_t> >;
template DLLExport class PropertyWithValue<std::vector<int32_t> >;
template DLLExport class PropertyWithValue<std::vector<uint64_t> >;
template DLLExport class PropertyWithValue<std::vector<int64_t> >;
/// @endcond

} // namespace Kernel
} // namespace Mantid
