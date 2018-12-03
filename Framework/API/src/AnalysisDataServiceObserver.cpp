// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/AnalysisDataServiceObserver.h"

AnalysisDataServiceObserver::AnalysisDataServiceObserver()
    : m_addObserver(*this, &AnalysisDataServiceObserver::_addHandle) {}

AnalysisDataServiceObserver::~AnalysisDataServiceObserver(){
  // Turn off/remove all observers
  this->observeAll(false);
}

void AnalysisDataServiceObserver::observeAll(bool turnOn) {
  this->observeAdd(turnOn);
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

void AnalysisDataServiceObserver::addHandle(
    const std::string &wsName, const Mantid::API::Workspace_sptr ws) {
  UNUSED_ARG(wsName)
  UNUSED_ARG(ws)
}