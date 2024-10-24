// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataServiceObserver.h"

namespace {
template <typename Observer> void modifyObserver(const bool turnOn, bool &isObserving, Observer &observer) {
  if (turnOn && !isObserving) {
    AnalysisDataService::Instance().notificationCenter.addObserver(observer);
  } else if (!turnOn && isObserving) {
    AnalysisDataService::Instance().notificationCenter.removeObserver(observer);
  }
  isObserving = turnOn;
}
} // namespace

namespace Mantid::API {

AnalysisDataServiceObserver::AnalysisDataServiceObserver()
    : m_addObserver(*this, &AnalysisDataServiceObserver::_addHandle),
      m_replaceObserver(*this, &AnalysisDataServiceObserver::_replaceHandle),
      m_deleteObserver(*this, &AnalysisDataServiceObserver::_deleteHandle),
      m_clearObserver(*this, &AnalysisDataServiceObserver::_clearHandle),
      m_renameObserver(*this, &AnalysisDataServiceObserver::_renameHandle),
      m_groupObserver(*this, &AnalysisDataServiceObserver::_groupHandle),
      m_unGroupObserver(*this, &AnalysisDataServiceObserver::_unGroupHandle),
      m_groupUpdatedObserver(*this, &AnalysisDataServiceObserver::_groupUpdateHandle) {}

AnalysisDataServiceObserver::~AnalysisDataServiceObserver() {
  // Turn off/remove all observers
  this->observeAll(false);
}

// ------------------------------------------------------------
// Observe Methods
// ------------------------------------------------------------

/**
 * @brief Function will turn on/off all observers for the ADS
 *
 * @param turnOn bool; if this is True, then if not already present, the
 * observer will be added else removed if it's false.
 */
void AnalysisDataServiceObserver::observeAll(bool turnOn) {
  this->observeAdd(turnOn);
  this->observeReplace(turnOn);
  this->observeDelete(turnOn);
  this->observeClear(turnOn);
  this->observeRename(turnOn);
  this->observeGroup(turnOn);
  this->observeUnGroup(turnOn);
  this->observeGroupUpdate(turnOn);
}

/**
 * @brief Function will add/remove the observer to the ADS for when a workspace
 * is added to it.
 *
 * @param turnOn bool; if this is True then, if not already present, the
 * observer will be added else removed if it's false.
 */
void AnalysisDataServiceObserver::observeAdd(bool turnOn) { modifyObserver(turnOn, m_observingAdd, m_addObserver); }

/**
 * @brief Function will add/remove the observer to the ADS for when a workspace
 * is replaced
 *
 * @param turnOn bool; if this is True then, if not already present, the
 * observer will be added else removed if it's false.
 */
void AnalysisDataServiceObserver::observeReplace(bool turnOn) {
  modifyObserver(turnOn, m_observingReplace, m_replaceObserver);
}

/**
 * @brief Function will add/remove the observer to the ADS for when a workspace
 * is deleted.
 *
 * @param turnOn bool; if this is True then, if not already present, the
 * observer will be added else removed if it's false.
 */
void AnalysisDataServiceObserver::observeDelete(bool turnOn) {
  modifyObserver(turnOn, m_observingDelete, m_deleteObserver);
}

/**
 * @brief Function will add/remove the observer to the ADS for when the ADS is
 * cleared.
 *
 * @param turnOn bool; if this is True then, if not already present, the
 * observer will be added else removed if it's false.
 */
void AnalysisDataServiceObserver::observeClear(bool turnOn) {
  modifyObserver(turnOn, m_observingClear, m_clearObserver);
}

/**
 * @brief Function will add/remove the observer to the ADS for when a workspace
 * is renamed
 *
 * @param turnOn bool; if this is True then, if not already present, the
 * observer will be added else removed if it's false.
 */
void AnalysisDataServiceObserver::observeRename(bool turnOn) {
  modifyObserver(turnOn, m_observingRename, m_renameObserver);
}

/**
 * @brief Function will add/remove the observer to the ADS for when a group is
 * added/created in the ADS
 *
 * @param turnOn bool; if this is True then, if not already present, the
 * observer will be added else removed if it's false.
 */
void AnalysisDataServiceObserver::observeGroup(bool turnOn) {
  modifyObserver(turnOn, m_observingGroup, m_groupObserver);
}

/**
 * @brief Function will add/remove the observer to the ADS for when a group is
 * removed/delete from the ADS
 *
 * @param turnOn bool; if this is True then, if not already present, the
 * observer will be added else removed if it's false.
 */
void AnalysisDataServiceObserver::observeUnGroup(bool turnOn) {
  modifyObserver(turnOn, m_observingUnGroup, m_unGroupObserver);
}

/**
 * @brief Function will add/remove the observer to the ADS for if a workspace is
 * added to a group or removed.
 *
 * @param turnOn bool; if this is True then, if not already present, the
 * observer will be added else removed if it's false.
 */
void AnalysisDataServiceObserver::observeGroupUpdate(bool turnOn) {
  modifyObserver(turnOn, m_observingGroupUpdate, m_groupUpdatedObserver);
}

// ------------------------------------------------------------
// Virtual Methods
// ------------------------------------------------------------
/**
 * @brief If anyChange to the ADS occurs then this function will trigger, works
 * by overloading this class and overriding this function.
 */
void AnalysisDataServiceObserver::anyChangeHandle() {}

/**
 * @brief If a workspace is added to the ADS, then this function will trigger,
 * works by overloading this class and overriding this function.
 *
 * @param wsName std::string; the name of the workspace added
 * @param ws Workspace_sptr; the Workspace that is added
 */
void AnalysisDataServiceObserver::addHandle(const std::string &wsName, const Mantid::API::Workspace_sptr &ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}

/**
 * @brief If a workspace is replaced in the ADS, then this function will
 * trigger, works by overloading this class and overriding this function
 *
 * @param wsName std::string; the name of the workspace replacing
 * @param ws Workspace_sptr; the Workspace that is replacing
 */
void AnalysisDataServiceObserver::replaceHandle(const std::string &wsName, const Mantid::API::Workspace_sptr &ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}

/**
 * @brief If a workspace is deleted from the ADS, then this function will
 * trigger, works by overloading this class and overriding this function
 *
 * @param wsName std::string; the name of the workspace
 * @param ws Workspace_sptr; the Workspace that is deleted
 */
void AnalysisDataServiceObserver::deleteHandle(const std::string &wsName, const Mantid::API::Workspace_sptr &ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}

/**
 * @brief If the ADS is cleared, then this function will trigger, works by
 * overloading this class and overriding this function
 */
void AnalysisDataServiceObserver::clearHandle() {}

/**
 * @brief If a workspace is renamed in the ADS, then this function will trigger,
 * works by overloading this class and overriding this function
 *
 * @param wsName std::string; the name of the workspace
 * @param newName std::string; the new name of the workspace
 */
void AnalysisDataServiceObserver::renameHandle(const std::string &wsName, const std::string &newName) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(newName)
}

/**
 * @brief If a group is created/added to the ADS, then this function will
 * trigger, works by overloading this class and overriding this function
 *
 * @param wsName std::string; the name of the workspace
 * @param ws Workspace_sptr; the WorkspaceGroup that was added/created
 */
void AnalysisDataServiceObserver::groupHandle(const std::string &wsName, const Workspace_sptr &ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}

/**
 * @brief If a group is removed from the ADS, then this function will trigger,
 * works by overloading this class and overriding this function
 *
 * @param wsName std::string; the name of the workspace
 * @param ws Workspace_sptr; the WorkspaceGroup that was ungrouped
 */
void AnalysisDataServiceObserver::unGroupHandle(const std::string &wsName, const Workspace_sptr &ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}

/**
 * @brief If a group has a workspace added/removed in the ADS, then this
 * function will trigger, works by overloading this class and overriding this
 * function.
 *
 * @param wsName std::string; the name of the workspace
 * @param ws Workspace_sptr; the WorkspaceGroup that was updated
 */
void AnalysisDataServiceObserver::groupUpdateHandle(const std::string &wsName, const Workspace_sptr &ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}

// ------------------------------------------------------------
// Private Methods
// ------------------------------------------------------------
void AnalysisDataServiceObserver::_addHandle(const Poco::AutoPtr<AnalysisDataServiceImpl::AddNotification> &pNf) {
  this->anyChangeHandle();
  this->addHandle(pNf->objectName(), pNf->object());
}

void AnalysisDataServiceObserver::_replaceHandle(
    const Poco::AutoPtr<AnalysisDataServiceImpl::AfterReplaceNotification> &pNf) {
  this->anyChangeHandle();
  this->replaceHandle(pNf->objectName(), pNf->object());
}

void AnalysisDataServiceObserver::_deleteHandle(
    const Poco::AutoPtr<AnalysisDataServiceImpl::PreDeleteNotification> &pNf) {
  this->anyChangeHandle();
  this->deleteHandle(pNf->objectName(), pNf->object());
}

void AnalysisDataServiceObserver::_clearHandle(const Poco::AutoPtr<AnalysisDataServiceImpl::ClearNotification> &pNf) {
  UNUSED_ARG(pNf)
  this->anyChangeHandle();
  this->clearHandle();
}

void AnalysisDataServiceObserver::_renameHandle(const Poco::AutoPtr<AnalysisDataServiceImpl::RenameNotification> &pNf) {
  this->anyChangeHandle();
  this->renameHandle(pNf->objectName(), pNf->newObjectName());
}

void AnalysisDataServiceObserver::_groupHandle(
    const Poco::AutoPtr<AnalysisDataServiceImpl::GroupWorkspacesNotification> &pNf) {
  this->anyChangeHandle();
  this->groupHandle(pNf->objectName(), pNf->object());
}

void AnalysisDataServiceObserver::_unGroupHandle(
    const Poco::AutoPtr<AnalysisDataServiceImpl::UnGroupingWorkspaceNotification> &pNf) {
  this->anyChangeHandle();
  this->unGroupHandle(pNf->objectName(), pNf->object());
}

void AnalysisDataServiceObserver::_groupUpdateHandle(
    const Poco::AutoPtr<AnalysisDataServiceImpl::GroupUpdatedNotification> &pNf) {
  this->anyChangeHandle();
  this->groupUpdateHandle(pNf->objectName(), pNf->object());
}

} // namespace Mantid::API
