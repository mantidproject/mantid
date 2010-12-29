#include "MantidKernel/PropertyWithValue.h"

namespace Mantid
{
namespace Kernel
{

/// @cond

template DLLExport class PropertyWithValue<int>;
template DLLExport class PropertyWithValue<bool>;
template DLLExport class PropertyWithValue<double>;
template DLLExport class PropertyWithValue<std::string>;

template DLLExport class PropertyWithValue<std::vector<int> >;
template DLLExport class PropertyWithValue<std::vector<double> >;
template DLLExport class PropertyWithValue<std::vector<std::string> >;
template DLLExport class PropertyWithValue<std::vector<long long> >;

/// @endcond

} // namespace Kernel
} // namespace Mantid
