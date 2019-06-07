// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "CatalogRunNotifier.h"

namespace MantidQt {
namespace CustomInterfaces {

CatalogRunNotifier::CatalogRunNotifier(IRunsView *view) : m_view(view) {
  m_view->subscribeTimer(this);
}

void CatalogRunNotifier::subscribe(RunNotifierSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void CatalogRunNotifier::startPolling() {
  m_view->startTimer(POLLING_INTERVAL_MILLISECONDS);
}

void CatalogRunNotifier::stopPolling() { m_view->stopTimer(); }

void CatalogRunNotifier::notifyTimerEvent() {
  if (m_notifyee)
    m_notifyee->notifyCheckForNewRuns();
}
} // namespace CustomInterfaces
} // namespace MantidQt
