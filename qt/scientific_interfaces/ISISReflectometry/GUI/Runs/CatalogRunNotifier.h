// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IRunNotifier.h"
#include "IRunsView.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
/** @class CatalogRunNotifier

CatalogRunNotifier implements IRunNotifier to provide functionality to
poll for new runs.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL CatalogRunNotifier : public IRunNotifier, public RunsViewTimerSubscriber {
public:
  static auto constexpr POLLING_INTERVAL_MILLISECONDS = 30000;

  explicit CatalogRunNotifier(IRunsView *view);
  ~CatalogRunNotifier() override = default;

  // IRunNotifier overrides
  void subscribe(RunNotifierSubscriber *notifyee) override;
  void startPolling() override;
  void stopPolling() override;

  // RunsViewTimerSubscriber overrides
  void notifyTimerEvent() override;

private:
  IRunsView *m_view;
  RunNotifierSubscriber *m_notifyee;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt