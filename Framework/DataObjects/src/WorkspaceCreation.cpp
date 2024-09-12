// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidIndexing/IndexInfo.h"

namespace Mantid::DataObjects::detail {
HistogramData::Histogram stripData(HistogramData::Histogram histogram) {
  histogram.setSharedY(nullptr);
  histogram.setSharedE(nullptr);
  return histogram;
}

template <> std::unique_ptr<EventWorkspace> createHelper() { return {nullptr}; }

template <> std::unique_ptr<API::HistoWorkspace> createHelper() { return std::make_unique<Workspace2D>(); }

template <> std::unique_ptr<API::MatrixWorkspace> createHelper() { return {nullptr}; }

template <> std::unique_ptr<API::MatrixWorkspace> createConcreteHelper() {
  throw std::runtime_error("Attempt to create instance of abstract type MatrixWorkspace");
}
template <> std::unique_ptr<API::HistoWorkspace> createConcreteHelper() {
  throw std::runtime_error("Attempt to create instance of abstract type HistoWorkspace");
}

template <class UseIndexInfo>
void doInitializeFromParent(const API::MatrixWorkspace &parent, API::MatrixWorkspace &workspace,
                            const bool differentSize) {
  API::WorkspaceFactory::Instance().initializeFromParent(parent, workspace, differentSize);
}

/** Same as WorkspaceFactory::initializeFromParent, with modifications for
 * changed IndexInfo.
 *
 * IndexInfo used for initialization this implies that the following data from
 * the parent is not applicable (since no automatic mapping possible):
 * - Bin masking
 * - Spectrum numbers and detector ID grouping
 * - Y axis
 */
template <>
void doInitializeFromParent<std::true_type>(const API::MatrixWorkspace &parent, API::MatrixWorkspace &child,
                                            const bool differentSize) {
  // Ignore flag since with IndexInfo the size is the same but we nevertheless
  // do not want to copy some data since spectrum order or definitions may have
  // changed. This should take care of not copying bin masks and Y axis.
  static_cast<void>(differentSize);

  const auto indexInfo = child.indexInfo();
  API::WorkspaceFactory::Instance().initializeFromParent(parent, child, true);
  // Restore previously set IndexInfo of child, undo changes to spectrum numbers
  // and detector ID grouping initializeFromParent does by default. This hack is
  // not optimal performance wise but copying data between workspaces is too
  // complicated and dangerous currently without using initializeFromParent.
  child.setIndexInfo(indexInfo);
}

/** Initialize a MatrixWorkspace from its parent including instrument, unit,
 * number of spectra and Run
 * @brief initializeFromParent
 * @param parent
 * @param ws
 */
template <class UseIndexInfo> void initializeFromParent(const API::MatrixWorkspace &parent, API::MatrixWorkspace &ws) {
  bool differentSize = (parent.x(0).size() != ws.x(0).size()) ||
                       (parent.id() != "EventWorkspace" && (parent.y(0).size() != ws.y(0).size()));
  doInitializeFromParent<UseIndexInfo>(parent, ws, differentSize);
  // For EventWorkspace, `ws.y(0)` put entry 0 in the MRU. However, clients
  // would typically expect an empty MRU and fail to clear it. This dummy call
  // removes the entry from the MRU.
  static_cast<void>(ws.mutableX(0));
}

template <> void fixDistributionFlag(API::MatrixWorkspace &workspace, const HistogramData::Histogram &histArg) {
  workspace.setDistribution(histArg.yMode() == HistogramData::Histogram::YMode::Frequencies);
}

template void MANTID_DATAOBJECTS_DLL initializeFromParent<std::true_type>(const API::MatrixWorkspace &,
                                                                          API::MatrixWorkspace &);
template void MANTID_DATAOBJECTS_DLL initializeFromParent<std::false_type>(const API::MatrixWorkspace &,
                                                                           API::MatrixWorkspace &);
} // namespace Mantid::DataObjects::detail
