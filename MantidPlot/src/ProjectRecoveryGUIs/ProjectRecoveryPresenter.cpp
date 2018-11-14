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
    : m_mainWindow(parentWindow), m_recView(nullptr), m_failureView(nullptr),
      m_model(std::make_unique<ProjectRecoveryModel>(projectRecovery, this)),
      m_openView(RecoveryView), m_startMantidNormallyCalled(false) {}

bool ProjectRecoveryPresenter::startRecoveryView() {
  try {
    m_recView = std::make_unique<ProjectRecoveryView>(m_mainWindow, this);
    m_openView = RecoveryView;
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
    m_failureView = std::make_unique<RecoveryFailureView>(m_mainWindow, this);
    m_openView = FailureView;
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
  const auto &vec = m_model->getRow(i);
  QStringList returnVal;
  for (auto i = 0u; i < vec.size(); ++i) {
    QString newString = QString::fromStdString(vec[i]);
    returnVal << newString;
  }
  return returnVal;
}

void ProjectRecoveryPresenter::recoverLast() {
  if (m_model->hasRecoveryStarted())
    return;
  auto checkpointToRecover = m_model->decideLastCheckpoint();
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

void ProjectRecoveryPresenter::recoverSelectedCheckpoint(
    const QString &selected) {
  if (m_model->hasRecoveryStarted())
    return;
  auto checkpointToRecover = selected.toStdString();
  m_model->recoverSelectedCheckpoint(checkpointToRecover);
}

void ProjectRecoveryPresenter::openSelectedInEditor(const QString &selected) {
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

void ProjectRecoveryPresenter::setUpProgressBar(const int barMax) {
  if (m_openView == RecoveryView && m_recView) {
    m_recView->setProgressBarMaximum(barMax);
  } else if (m_failureView) {
    m_failureView->setProgressBarMaximum(barMax);
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

void ProjectRecoveryPresenter::changeStartMantidToCancelLabel() {
  if (m_openView == RecoveryView) {
    m_recView->changeStartMantidButton("Cancel Recovery");
  } else {
    m_failureView->changeStartMantidButton("Cancel Recovery");
  }
}

void ProjectRecoveryPresenter::fillAllRows() {
  // Only allow this to run once, first run will have value RecoveryView
  if (m_openView == RecoveryView) {
    m_model->fillRows();
  }
}

int ProjectRecoveryPresenter::getNumberOfCheckpoints() {
  return ProjectRecoveryModel::getNumberOfCheckpoints();
}