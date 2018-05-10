//-----------------------------------
// Includes
//-----------------------------------
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidAPI/AnalysisDataService.h"

namespace MantidQt {
namespace API {

//---------------------------------------------------------------------------
// Observer callback
//---------------------------------------------------------------------------
void ObserverCallback::handlePreDelete(const std::string &name,
                                       Mantid::API::Workspace_sptr workspace) {
  m_observer->preDeleteHandle(name, workspace);
}

void ObserverCallback::handlePostDelete(const std::string &name) {
  m_observer->postDeleteHandle(name);
}

void ObserverCallback::handleAdd(const std::string &name,
                                 Mantid::API::Workspace_sptr workspace) {
  m_observer->addHandle(name, workspace);
}

void ObserverCallback::handleAfterReplace(
    const std::string &name, Mantid::API::Workspace_sptr workspace) {
  m_observer->afterReplaceHandle(name, workspace);
}

void ObserverCallback::handleRename(const std::string &oldName,
                                    const std::string &newName) {
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
      m_clearADSObserver(*this, &WorkspaceObserver::_clearADSHandle),
      m_predel_observed(false), m_postdel_observed(false),
      m_add_observed(false), m_repl_observed(false), m_rename_observed(false),
      m_clr_observed(false) {
  m_proxy = new ObserverCallback(this);
}

/// Destructor
WorkspaceObserver::~WorkspaceObserver() {
  observePreDelete(false);
  observePostDelete(false);
  observeAdd(false);
  observeAfterReplace(false);
  observeRename(false);
  observeADSClear(false);

  m_proxy->disconnect();
  delete m_proxy;
}

/**
 * Turn on observations of workspace delete notifications from the ADS
 * @param turnOn :: If true observe the notifications, otherwise disable
 * observation [default=true]
 */
void WorkspaceObserver::observePreDelete(bool turnOn) {
  if (turnOn && !m_predel_observed) // Turning it on
  {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(
        m_preDeleteObserver);
    m_proxy->connect(
        m_proxy,
        SIGNAL(preDeleteRequested(const std::string &,
                                  Mantid::API::Workspace_sptr)),
        SLOT(handlePreDelete(const std::string &, Mantid::API::Workspace_sptr)),
        Qt::QueuedConnection);
  } else if (!turnOn && m_predel_observed) // Turning it off
  {
    Mantid::API::AnalysisDataService::Instance()
        .notificationCenter.removeObserver(m_preDeleteObserver);
    m_proxy->disconnect(m_proxy,
                        SIGNAL(preDeleteRequested(const std::string &,
                                                  Mantid::API::Workspace_sptr)),
                        m_proxy,
                        SLOT(handlePreDelete(const std::string &,
                                             Mantid::API::Workspace_sptr)));
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
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(
        m_postDeleteObserver);
    m_proxy->connect(m_proxy, SIGNAL(postDeleteRequested(const std::string &)),
                     SLOT(handlePostDelete(const std::string &)),
                     Qt::QueuedConnection);
  } else if (!turnOn && m_postdel_observed) // Turning it off
  {
    Mantid::API::AnalysisDataService::Instance()
        .notificationCenter.removeObserver(m_postDeleteObserver);
    m_proxy->disconnect(m_proxy,
                        SIGNAL(postDeleteRequested(const std::string &)),
                        m_proxy, SLOT(handlePostDelete(const std::string &)));
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
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(
        m_afterReplaceObserver);
    m_proxy->connect(
        m_proxy,
        SIGNAL(afterReplaced(const std::string &, Mantid::API::Workspace_sptr)),
        SLOT(handleAfterReplace(const std::string &,
                                Mantid::API::Workspace_sptr)),
        Qt::QueuedConnection);
  } else if (!turnOn && m_repl_observed) {
    Mantid::API::AnalysisDataService::Instance()
        .notificationCenter.removeObserver(m_afterReplaceObserver);
    m_proxy->disconnect(
        m_proxy,
        SIGNAL(afterReplaced(const std::string &, Mantid::API::Workspace_sptr)),
        m_proxy,
        SLOT(handleAfterReplace(const std::string &,
                                Mantid::API::Workspace_sptr)));
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
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(
        m_renameObserver);
    m_proxy->connect(
        m_proxy, SIGNAL(renamed(const std::string &, const std::string &)),
        SLOT(handleRename(const std::string &, const std::string &)),
        Qt::QueuedConnection);
  } else if (!turnOn && m_rename_observed) {
    Mantid::API::AnalysisDataService::Instance()
        .notificationCenter.removeObserver(m_renameObserver);
    m_proxy->disconnect(
        m_proxy, SIGNAL(renamed(const std::string &, const std::string &)),
        m_proxy, SLOT(handleRename(const std::string &, const std::string &)));
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
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(
        m_addObserver);
    m_proxy->connect(
        m_proxy,
        SIGNAL(addRequested(const std::string &, Mantid::API::Workspace_sptr)),
        SLOT(handleAdd(const std::string &, Mantid::API::Workspace_sptr)),
        Qt::QueuedConnection);
  } else if (!turnOn && m_add_observed) {
    Mantid::API::AnalysisDataService::Instance()
        .notificationCenter.removeObserver(m_addObserver);
    m_proxy->disconnect(
        m_proxy,
        SIGNAL(addRequested(const std::string &, Mantid::API::Workspace_sptr)),
        m_proxy,
        SLOT(handleAdd(const std::string &, Mantid::API::Workspace_sptr)));
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
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(
        m_clearADSObserver);
    m_proxy->connect(m_proxy, SIGNAL(adsCleared()), SLOT(handleClearADS()),
                     Qt::QueuedConnection);
  } else if (!turnOn && m_clr_observed) {
    Mantid::API::AnalysisDataService::Instance()
        .notificationCenter.removeObserver(m_clearADSObserver);
    m_proxy->disconnect(m_proxy, SIGNAL(adsCleared()), m_proxy,
                        SLOT(handleClearADS()));
  }
  m_clr_observed = turnOn;
}

} // namespace API
} // namespace MantidQt
