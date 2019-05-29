// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_CATALOGRUNNOTIFIER_H
#define MANTID_ISISREFLECTOMETRY_CATALOGRUNNOTIFIER_H

#include "GUI/MainWindow/IMainWindowView.h"
#include "IRunNotifier.h"

namespace MantidQt {
namespace CustomInterfaces {
/** @class CatalogRunNotifier

CatalogRunNotifier implements IRunNotifier to provide functionality to
poll for new runs.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL CatalogRunNotifier
    : public IRunNotifier,
      public MainWindowSubscriber {
public:
  static auto constexpr POLLING_INTERVAL_MILLISECONDS = 5000;

  explicit CatalogRunNotifier(IMainWindowView *view);
  ~CatalogRunNotifier() override{};

  void subscribe(RunNotifierSubscriber *notifyee) override;

  // IRunNotifier overrides
  void startPolling() override;
  void stopPolling() override;

  // MainWindowSubscriber overrides
  void notifyTimerEvent() override;

private:
  IMainWindowView *m_view;
  RunNotifierSubscriber *m_notifyee;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif
