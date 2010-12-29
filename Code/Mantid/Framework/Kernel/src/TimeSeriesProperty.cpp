#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Exception.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <list>
#include <numeric>
#include <vector>

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


