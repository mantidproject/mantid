#include "MantidKernel/make_unique.h"
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

std::unique_ptr<Workspace2D> createWorkspace2D() {
  return Kernel::make_unique<Workspace2D>();
}
}
} // namespace DataObjects
} // namespace Mantid
