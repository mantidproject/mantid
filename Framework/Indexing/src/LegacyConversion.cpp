#include "MantidIndexing/LegacyConversion.h"

namespace Mantid {
namespace Indexing {

/// Converts a vector of specnum_t (int32_t) to vector of SpectrumNumber.
std::vector<SpectrumNumber>
makeSpectrumNumberVector(const std::vector<int32_t> &data) {
  return {data.begin(), data.end()};
}

} // namespace Indexing
} // namespace Mantid
