// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/AnalysisDataServiceObserver.h"

AnalysisDataServiceObserver::AnalysisDataServiceObserver()
    : m_addObserver(*this, &AnalysisDataServiceObserver::_addHandle) {}

AnalysisDataServiceObserver::~AnalysisDataServiceObserver() {
  // Turn off/remove all observers
  this->observeAll(false);
}


// ------------------------------------------------------------
// Observe Methods
// ------------------------------------------------------------

void AnalysisDataServiceObserver::observeAll(bool turnOn) {
  this->observeAdd(turnOn);
  this->observeReplace(turnOn);
  this->observeDelete(turnOn);
}

void AnalysisDataServiceObserver::observeAdd(bool turnOn) {
  if (turnOn && !m_observingAdd) {
    AnalysisDataService::Instance().notificationCenter.addObserver(
        m_addObserver);
  } else if (!turnOn && m_observingAdd) {
    AnalysisDataService::Instance().notificationCenter.removeObserver(
        m_addObserver);
  }
  m_observingAdd = turnOn;
}

void AnalysisDataServiceObserver::observeReplace(bool turnOn) {
  if (turnOn && !m_observingReplace) {
    AnalysisDataService::Instance().notificationCenter.addObserver(
        m_ReplaceObserver);
  } else if (!turnOn && m_observingReplace) {
    AnalysisDataService::Instance().notificationCenter.removeObserver(
        m_ReplaceObserver);
  }
  m_observingReplace = turnOn;
}

void AnalysisDataServiceObserver::observeDelete(bool turnOn) {
  if (turnOn && !m_observingDelete) {
    AnalysisDataService::Instance().notificationCenter.addObserver(
        m_deleteObserver);
  } else if (!turnOn && m_observingDelete) {
    AnalysisDataService::Instance().notificationCenter.removeObserver(
        m_deleteObserver);
  }
  m_observingDelete = turnOn;
}

// ------------------------------------------------------------
// Virtual Methods
// ------------------------------------------------------------
void AnalysisDataServiceObserver::addHandle(
    const std::string &wsName, const Mantid::API::Workspace_sptr ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}

void AnalysisDataServiceObserver::replaceHandle(
    const std::string &wsName, const Mantid::API::Workspace_sptr ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}

void AnalysisDataServiceObserver::deleteHandle(
    const std::string &wsName, const Mantid::API::Workspace_sptr ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}

// ------------------------------------------------------------
// Private Methods
// ------------------------------------------------------------
void AnalysisDataServiceObserver::_addHandle(
    const Poco::AutoPtr<AnalysisDataServiceImpl::AddNotification> &pNf) {
  this->anyChangeHandle();
  this->addHandle(pNf->objectName(), pNf->object());
}

void AnalysisDataServiceObserver::_replaceHandle(
    const Poco::AutoPtr<AnalysisDataServiceImpl::AfterReplaceNotification>
        &pNf) {
  this->anyChangeHandle();
  this->replaceHandle(pNf->objectName(), pNf->object());
}

void AnalysisDataServiceObserver::_deleteHandle(
    const Poco::AutoPtr<AnalysisDataServiceImpl::AfterReplaceNotification>
        &pNf) {
  this->anyChangeHandle();
  this->deleteHandle(pNf->objectName(), pNf->object());
}
