#include "MantidKernel/make_unique.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataObjects {
namespace detail {
HistogramData::Histogram stripData(HistogramData::Histogram histogram) {
  histogram.setSharedY(nullptr);
  histogram.setSharedE(nullptr);
  return histogram;
}

template <> std::unique_ptr<EventWorkspace> createHelper() {
  return {nullptr};
}

template <> std::unique_ptr<API::HistoWorkspace> createHelper() {
  return Kernel::make_unique<Workspace2D>();
}

template <> std::unique_ptr<API::MatrixWorkspace> createHelper() {
  return {nullptr};
}
}
} // namespace DataObjects
} // namespace Mantid
