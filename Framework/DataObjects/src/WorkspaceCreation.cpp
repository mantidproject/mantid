#include "MantidKernel/make_unique.h"
#include "MantidAPI/WorkspaceFactory.h"
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

template <> std::unique_ptr<EventWorkspace> createHelper() { return {nullptr}; }

template <> std::unique_ptr<API::HistoWorkspace> createHelper() {
  return Kernel::make_unique<Workspace2D>();
}

template <> std::unique_ptr<API::MatrixWorkspace> createHelper() {
  return {nullptr};
}

template <> std::unique_ptr<API::MatrixWorkspace> createConcreteHelper() {
  throw std::runtime_error(
      "Attempt to create instance of abstract type MatrixWorkspace");
  return {nullptr};
}
template <> std::unique_ptr<API::HistoWorkspace> createConcreteHelper() {
  throw std::runtime_error(
      "Attempt to create instance of abstract type HistoWorkspace");
  return {nullptr};
}

/** Initialize a MatrixWorkspace from its parent including instrument, unit,
 * number of spectra and Run
 * @brief initializeFromParent
 * @param parent
 * @param ws
 */
void initializeFromParent(const API::MatrixWorkspace &parent,
                          API::MatrixWorkspace &ws) {
  bool differentSize = (parent.x(0).size() != ws.x(0).size()) ||
                       (parent.y(0).size() != ws.y(0).size());
  API::WorkspaceFactory::Instance().initializeFromParent(parent, ws,
                                                         differentSize);
  // For EventWorkspace, `ws.y(0)` put entry 0 in the MRU. However, clients
  // would typically expect an empty MRU and fail to clear it. This dummy call
  // removes the entry from the MRU.
  static_cast<void>(ws.mutableX(0));
}

/** Initialize a MatrixWorkspace from its parent including instrument, unit,
 * number of spectra but without Run (i.e., logs)
 * @brief initializeFromParentWithoutLogs
 * @param parent
 * @param ws
 */
void initializeFromParentWithoutLogs(const API::MatrixWorkspace &parent,
                                     API::MatrixWorkspace &ws) {
  bool differentSize = (parent.x(0).size() != ws.x(0).size()) ||
                       (parent.y(0).size() != ws.y(0).size());
  API::WorkspaceFactory::Instance().initializeFromParentWithoutLogs(
      parent, ws, differentSize);
  // For EventWorkspace, `ws.y(0)` put entry 0 in the MRU. However, clients
  // would typically expect an empty MRU and fail to clear it. This dummy call
  // removes the entry from the MRU.
  static_cast<void>(ws.mutableX(0));
}

template <>
void fixDistributionFlag(API::MatrixWorkspace &workspace,
                         const HistogramData::Histogram &histArg) {
  workspace.setDistribution(histArg.yMode() ==
                            HistogramData::Histogram::YMode::Frequencies);
}
}
} // namespace DataObjects
} // namespace Mantid
