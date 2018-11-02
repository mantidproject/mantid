// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ProjectRecoveryPresenter.h"
#include "ApplicationWindow.h"
#include "ProjectRecovery.h"
#include "ProjectRecoveryModel.h"
#include "ProjectRecoveryView.h"
#include "RecoveryFailureView.h"

#include <QDialog>
#include <memory>

ProjectRecoveryPresenter::ProjectRecoveryPresenter(
    MantidQt::ProjectRecovery *projectRecovery, ApplicationWindow *parentWindow)
    : m_mainWindow(parentWindow) {
  m_recView = nullptr;
  m_failureView = nullptr;
  m_model = new ProjectRecoveryModel(projectRecovery, this);
  m_openView = RecoveryView;
  m_startMantidNormallyCalled = false;
}

ProjectRecoveryPresenter::ProjectRecoveryPresenter(
    const ProjectRecoveryPresenter &obj) {
  /// Copy constructor can only copy the Model
  m_recView = obj.m_recView;
  m_failureView = obj.m_failureView;
  m_model = new ProjectRecoveryModel(nullptr, this);
  *m_model = *obj.m_model;
  m_mainWindow = obj.m_mainWindow;
  m_openView = obj.m_openView;
  m_startMantidNormallyCalled = obj.m_startMantidNormallyCalled;
}

ProjectRecoveryPresenter::~ProjectRecoveryPresenter() {
  delete m_recView;
  delete m_failureView;
  delete m_model;
}

bool ProjectRecoveryPresenter::startRecoveryView() {
  try {
    m_openView = RecoveryView;
    m_recView = new ProjectRecoveryView(m_mainWindow, this);
    m_recView->exec();
  } catch (...) {
    return true;
  }

  // If start mantid normally was called we want to cancel
  if (m_startMantidNormallyCalled) {
    return false;
  }

  // If run has failed and recovery is not running
  if (m_model->getFailedRun()) {
    return true;
  }
  return false;
}

bool ProjectRecoveryPresenter::startRecoveryFailure() {
  try {
    m_openView = FailureView;
    m_failureView = new RecoveryFailureView(m_mainWindow, this);
    m_failureView->exec();
  } catch (...) {
    return true;
  }

  // If start mantid normally was called we want to cancel
  if (m_startMantidNormallyCalled) {
    return false;
  }

  // If run has failed and recovery is not running
  if (m_model->getFailedRun()) {
    return true;
  }
  return false;
}

QStringList ProjectRecoveryPresenter::getRow(int i) {
  auto vec = m_model->getRow(i);
  QStringList returnVal;
  for (auto i = 0; i < 3; ++i) {
    QString newString = QString::fromStdString(vec[i]);
    returnVal << newString;
  }
  return returnVal;
}

void ProjectRecoveryPresenter::recoverLast() {
  if (m_model->hasRecoveryStarted())
    return;
  auto checkpointToRecover = m_model->decideLastCheckpoint();
  setUpProgressBar(checkpointToRecover);
  m_model->recoverSelectedCheckpoint(checkpointToRecover);
}

void ProjectRecoveryPresenter::openLastInEditor() {
  if (m_model->hasRecoveryStarted())
    return;
  auto checkpointToRecover = m_model->decideLastCheckpoint();
  m_model->openSelectedInEditor(checkpointToRecover);
}

void ProjectRecoveryPresenter::startMantidNormally() {
  m_startMantidNormallyCalled = true;
  m_model->startMantidNormally();
}

void ProjectRecoveryPresenter::recoverSelectedCheckpoint(QString &selected) {
  if (m_model->hasRecoveryStarted())
    return;
  auto checkpointToRecover = selected.toStdString();
  setUpProgressBar(checkpointToRecover);
  m_model->recoverSelectedCheckpoint(checkpointToRecover);
}

void ProjectRecoveryPresenter::openSelectedInEditor(QString &selected) {
  if (m_model->hasRecoveryStarted())
    return;
  auto checkpointToRecover = selected.toStdString();
  m_model->openSelectedInEditor(checkpointToRecover);
}

void ProjectRecoveryPresenter::closeView() {
  if (m_recView != nullptr) {
    m_recView->setVisible(false);
  }
  if (m_failureView != nullptr) {
    m_failureView->setVisible(false);
  }
}

ProjectRecoveryPresenter &ProjectRecoveryPresenter::
operator=(const ProjectRecoveryPresenter &obj) {
  if (&obj != this) {
    m_recView = obj.m_recView;
    m_failureView = obj.m_failureView;
    m_model = new ProjectRecoveryModel(nullptr, this);
    *m_model = *obj.m_model;
    m_mainWindow = obj.m_mainWindow;
    m_openView = obj.m_openView;
    m_startMantidNormallyCalled = obj.m_startMantidNormallyCalled;
  }
  return *this;
}

void ProjectRecoveryPresenter::setUpProgressBar(
    std::string checkpointToRecover) {
  auto row = m_model->getRow(checkpointToRecover);
  if (m_openView == RecoveryView && m_recView) {
    m_recView->setProgressBarMaximum(std::stoi(row[1]) + 1);
  } else if (m_failureView) {
    m_failureView->setProgressBarMaximum(std::stoi(row[1]) + 1);
  }
}

void ProjectRecoveryPresenter::connectProgressBarToRecoveryView() {
  if (m_openView == RecoveryView) {
    m_recView->connectProgressBar();
  } else {
    m_failureView->connectProgressBar();
  }
}

void ProjectRecoveryPresenter::emitAbortScript() {
  if (m_openView == RecoveryView) {
    m_recView->emitAbortScript();
  } else {
    m_failureView->emitAbortScript();
  }
}

void ProjectRecoveryPresenter::changeStartMantidToCancelLabel(){
  if (m_openView == RecoveryView) {
    m_recView->changeStartMantidButton("Cancel Recovery");
  } else {
    m_failureView->changeStartMantidButton("Cancel Recovery");
  }
}