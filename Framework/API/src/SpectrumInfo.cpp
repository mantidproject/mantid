#include "MantidAPI/GeometryInfoFactory.h"
#include "MantidAPI/GeometryInfo.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace API {

SpectrumInfo::SpectrumInfo(const MatrixWorkspace &workspace)
    : m_factory{Kernel::make_unique<GeometryInfoFactory>(workspace)},
      m_info(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {}

// Defined as default in source for forward declaration with std::unique_ptr.
SpectrumInfo::SpectrumInfo(SpectrumInfo &&) = default;
SpectrumInfo::~SpectrumInfo() = default;

bool SpectrumInfo::isMonitor(const size_t index) const {
  return getInfo(index).isMonitor();
}

bool SpectrumInfo::isMasked(const size_t index) const {
  return getInfo(index).isMasked();
}

const GeometryInfo &SpectrumInfo::getInfo(const size_t index) const {
  updateCachedInfo(index);
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  return *m_info[thread];
}

void SpectrumInfo::updateCachedInfo(const size_t index) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastIndex[thread] == index)
    return;
  m_lastIndex[thread] = index;
  m_info[thread] = std::make_unique<GeometryInfo>(m_factory->create(index));
}

} // namespace API
} // namespace Mantid
