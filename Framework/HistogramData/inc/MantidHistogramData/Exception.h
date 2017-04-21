#include "MantidHistogramData/DllConfig.h"
#include <stdexcept>

namespace Mantid {
namespace HistogramData {
namespace Exception {

/// Thrown whenever invalid bin edges are encountered by HistogramData Rebin
class MANTID_HISTOGRAMDATA_DLL InvalidBinEdgesError
    : public std::runtime_error {
public:
  InvalidBinEdgesError(const char *what);
};
}
}
}