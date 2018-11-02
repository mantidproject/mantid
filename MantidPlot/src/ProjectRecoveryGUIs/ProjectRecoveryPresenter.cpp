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

void ProjectRecoveryPresenter::recoverLast(boost::shared_ptr<QDialog> view) {
  auto checkpointToRecover = m_model->decideLastCheckpoint();
  setUpProgressBar(checkpointToRecover, view);
  m_model->recoverSelectedCheckpoint(checkpointToRecover);
}

void ProjectRecoveryPresenter::openLastInEditor() {
  auto checkpointToRecover = m_model->decideLastCheckpoint();
  m_model->openSelectedInEditor(checkpointToRecover);
}

void ProjectRecoveryPresenter::startMantidNormally() {
  m_model->startMantidNormally();
}

void ProjectRecoveryPresenter::recoverSelectedCheckpoint(
    QString &selected, boost::shared_ptr<QDialog> view) {
  auto checkpointToRecover = selected.toStdString();
  setUpProgressBar(checkpointToRecover, view);
  m_model->recoverSelectedCheckpoint(checkpointToRecover);
}

void ProjectRecoveryPresenter::openSelectedInEditor(QString &selected) {
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
  }
  return *this;
}

void ProjectRecoveryPresenter::setUpProgressBar(
    std::string checkpointToRecover, boost::shared_ptr<QDialog> view) {
  auto row = m_model->getRow(checkpointToRecover);
  auto firstView = boost::dynamic_pointer_cast<ProjectRecoveryView>(view);
  auto secondView = boost::dynamic_pointer_cast<RecoveryFailureView>(view);
  if (firstView) {
    firstView->setProgressBarMaximum(std::stoi(row[2]));
  }
  if (secondView) {
    secondView->setProgressBarMaximum(std::stoi(row[2]));
  }
}

void ProjectRecoveryPresenter::connectProgressBarToRecoveryView() {
  if (m_openView == RecoveryView) {
    m_recView->connectProgressBar();
  } else {
    m_failureView->connectProgressBar();
  }
}