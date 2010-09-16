#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid
{
namespace Kernel
{

  //Logger& TimeSeriesProperty::g_log = Logger::get("TimeSeriesProperty");

/// @cond

template DLLExport class TimeSeriesProperty<double>;
template DLLExport class TimeSeriesProperty<std::string>;

/// @endcond

} // namespace Kernel
} // namespace Mantid


