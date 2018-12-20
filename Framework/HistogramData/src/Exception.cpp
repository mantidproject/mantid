#include "MantidHistogramData/Exception.h"

namespace Mantid {
namespace HistogramData {
namespace Exception {
InvalidBinEdgesError::InvalidBinEdgesError(const char *what)
    : runtime_error(what) {}
} // namespace Exception
} // namespace HistogramData
} // namespace Mantid