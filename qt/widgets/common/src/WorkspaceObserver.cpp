// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------
// Includes
//-----------------------------------
#include <utility>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AnalysisDataServiceWrapper.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"

namespace MantidQt::API {

//---------------------------------------------------------------------------
// Observer callback
//---------------------------------------------------------------------------
void ObserverCallback::handlePreDelete(const std::string &name, const Mantid::API::Workspace_sptr &workspace) {
  m_observer->preDeleteHandle(name, workspace);
}

void ObserverCallback::handlePostDelete(const std::string &name) { m_observer->postDeleteHandle(name); }

void ObserverCallback::handleAdd(const std::string &name, const Mantid::API::Workspace_sptr &workspace) {
  m_observer->addHandle(name, workspace);
}

void ObserverCallback::handleAfterReplace(const std::string &name, const Mantid::API::Workspace_sptr &workspace) {
  m_observer->afterReplaceHandle(name, workspace);
}

void ObserverCallback::handleRename(const std::string &oldName, const std::string &newName) {
  m_observer->renameHandle(oldName, newName);
}

void ObserverCallback::handleClearADS() { m_observer->clearADSHandle(); }

//---------------------------------------------------------------------------
// WorkspaceObserver
//---------------------------------------------------------------------------

/// Default constructor
WorkspaceObserver::WorkspaceObserver()
    : m_preDeleteObserver(*this, &WorkspaceObserver::_preDeleteHandle),
      m_postDeleteObserver(*this, &WorkspaceObserver::_postDeleteHandle),
      m_addObserver(*this, &WorkspaceObserver::_addHandle),
      m_afterReplaceObserver(*this, &WorkspaceObserver::_afterReplaceHandle),
      m_renameObserver(*this, &WorkspaceObserver::_renameHandle),
      m_clearADSObserver(*this, &WorkspaceObserver::_clearADSHandle), m_proxy(std::make_unique<ObserverCallback>(this)),
      m_predel_observed(false), m_postdel_observed(false), m_add_observed(false), m_repl_observed(false),
      m_rename_observed(false), m_clr_observed(false) {}

/// Destructor
WorkspaceObserver::~WorkspaceObserver() {
  auto &notificationCenter = Mantid::API::AnalysisDataService::Instance().getNotificationCenter();
  // Do the minimum cleanup of dangling observer refs, rather than disconnecting signals / slots
  // through observeX, as we're about to destroy the proxy anyway
  notificationCenter.removeObserver(m_preDeleteObserver);
  notificationCenter.removeObserver(m_postDeleteObserver);
  notificationCenter.removeObserver(m_addObserver);
  notificationCenter.removeObserver(m_afterReplaceObserver);
  notificationCenter.removeObserver(m_renameObserver);
  notificationCenter.removeObserver(m_clearADSObserver);

  m_proxy->disconnect();
}

/**
 * Turn on observations of workspace delete notifications from the ADS
 * @param turnOn :: If true observe the notifications, otherwise disable
 * observation [default=true]
 */
void WorkspaceObserver::observePreDelete(bool turnOn) {
  if (turnOn && !m_predel_observed) // Turning it on
  {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().addObserver(m_preDeleteObserver);
    m_proxy->connect(m_proxy.get(), SIGNAL(preDeleteRequested(const std::string &, Mantid::API::Workspace_sptr)),
                     SLOT(handlePreDelete(const std::string &, Mantid::API::Workspace_sptr)), Qt::QueuedConnection);
  } else if (!turnOn && m_predel_observed) // Turning it off
  {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().removeObserver(m_preDeleteObserver);
    m_proxy->disconnect(m_proxy.get(), SIGNAL(preDeleteRequested(const std::string &, Mantid::API::Workspace_sptr)),
                        m_proxy.get(), SLOT(handlePreDelete(const std::string &, Mantid::API::Workspace_sptr)));
  } else {
  }
  m_predel_observed = turnOn;
}

/**
 * Turn on observations of workspace post delete notifications from the ADS
 * @param turnOn :: If true observe the notifications, other wise disable
 * observation [default=true]
 */
void WorkspaceObserver::observePostDelete(bool turnOn) {
  if (turnOn && !m_postdel_observed) // Turning it on
  {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().addObserver(m_postDeleteObserver);
    m_proxy->connect(m_proxy.get(), SIGNAL(postDeleteRequested(const std::string &)),
                     SLOT(handlePostDelete(const std::string &)), Qt::QueuedConnection);
  } else if (!turnOn && m_postdel_observed) // Turning it off
  {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().removeObserver(m_postDeleteObserver);
    m_proxy->disconnect(m_proxy.get(), SIGNAL(postDeleteRequested(const std::string &)), m_proxy.get(),
                        SLOT(handlePostDelete(const std::string &)));
  } else {
  }
  m_postdel_observed = turnOn;
}

/**
 * Turn on observations of workspace replacement notifications from the ADS
 * @param turnOn :: If true observe the notifications, otherwise disable
 * observation [default=true]
 */
void WorkspaceObserver::observeAfterReplace(bool turnOn) {
  if (turnOn && !m_repl_observed) {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().addObserver(m_afterReplaceObserver);
    m_proxy->connect(m_proxy.get(), SIGNAL(afterReplaced(const std::string &, Mantid::API::Workspace_sptr)),
                     SLOT(handleAfterReplace(const std::string &, Mantid::API::Workspace_sptr)), Qt::QueuedConnection);
  } else if (!turnOn && m_repl_observed) {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().removeObserver(m_afterReplaceObserver);
    m_proxy->disconnect(m_proxy.get(), SIGNAL(afterReplaced(const std::string &, Mantid::API::Workspace_sptr)),
                        m_proxy.get(), SLOT(handleAfterReplace(const std::string &, Mantid::API::Workspace_sptr)));
  }
  m_repl_observed = turnOn;
}

/**
 * Turn on observations of workspace renaming notifications from the ADS
 * @param turnOn :: If true observe the notifications, otherwise disable
 * observation [default=true]
 */
void WorkspaceObserver::observeRename(bool turnOn) {
  if (turnOn && !m_rename_observed) {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().addObserver(m_renameObserver);
    m_proxy->connect(m_proxy.get(), SIGNAL(renamed(const std::string &, const std::string &)),
                     SLOT(handleRename(const std::string &, const std::string &)), Qt::QueuedConnection);
  } else if (!turnOn && m_rename_observed) {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().removeObserver(m_renameObserver);
    m_proxy->disconnect(m_proxy.get(), SIGNAL(renamed(const std::string &, const std::string &)), m_proxy.get(),
                        SLOT(handleRename(const std::string &, const std::string &)));
  }
  m_rename_observed = turnOn;
}

/**
 * Turn on observations of workspace add notifications from the ADS
 * @param turnOn :: If true observe the notifications, otherwise disable
 * observation [default=true]
 */
void WorkspaceObserver::observeAdd(bool turnOn) {
  if (turnOn && !m_add_observed) {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().addObserver(m_addObserver);
    m_proxy->connect(m_proxy.get(), SIGNAL(addRequested(const std::string &, Mantid::API::Workspace_sptr)),
                     SLOT(handleAdd(const std::string &, Mantid::API::Workspace_sptr)), Qt::QueuedConnection);
  } else if (!turnOn && m_add_observed) {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().removeObserver(m_addObserver);
    m_proxy->disconnect(m_proxy.get(), SIGNAL(addRequested(const std::string &, Mantid::API::Workspace_sptr)),
                        m_proxy.get(), SLOT(handleAdd(const std::string &, Mantid::API::Workspace_sptr)));
  }
  m_add_observed = turnOn;
}

/**
 * Turn on observations of workspace clear notifications from the ADS
 * @param turnOn :: If true observe the notifications, otherwise disable
 * observation [default=true]
 */
void WorkspaceObserver::observeADSClear(bool turnOn) {
  if (turnOn && !m_clr_observed) {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().addObserver(m_clearADSObserver);
    m_proxy->connect(m_proxy.get(), SIGNAL(adsCleared()), SLOT(handleClearADS()), Qt::QueuedConnection);
  } else if (!turnOn && m_clr_observed) {
    Mantid::API::AnalysisDataService::Instance().getNotificationCenter().removeObserver(m_clearADSObserver);
    m_proxy->disconnect(m_proxy.get(), SIGNAL(adsCleared()), m_proxy.get(), SLOT(handleClearADS()));
  }
  m_clr_observed = turnOn;
}

} // namespace MantidQt::API
