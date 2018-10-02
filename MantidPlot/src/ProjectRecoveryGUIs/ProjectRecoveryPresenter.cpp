#include "ProjectRecoveryPresenter.h"
#include "ApplicationWindow.h"
#include "ProjectRecovery.h"
#include "ProjectRecoveryModel.h"
#include "ProjectRecoveryView.h"
#include "RecoveryFailureView.h"

#include <memory>

ProjectRecoveryPresenter::ProjectRecoveryPresenter(
    MantidQt::ProjectRecovery *projectRecovery, ApplicationWindow *parentWindow)
    : m_mainWindow(parentWindow) {
  m_recView = nullptr;
  m_failureView = nullptr;
  m_model = new ProjectRecoveryModel(projectRecovery, this);
}

ProjectRecoveryPresenter::ProjectRecoveryPresenter(
    const ProjectRecoveryPresenter &obj) {
  /// Copy constructor can only copy the Model
  m_recView = obj.m_recView;
  m_failureView = obj.m_failureView;
  m_model = obj.m_model;
  m_mainWindow = obj.m_mainWindow;
}

ProjectRecoveryPresenter::~ProjectRecoveryPresenter() {
  delete m_recView;
  delete m_failureView;
  delete m_model;
}

bool ProjectRecoveryPresenter::startRecoveryView() {
  try {
    m_recView = new ProjectRecoveryView(m_mainWindow, this);
    m_recView->exec();
  } catch (...) {
    return true;
  }
  return false;
}

bool ProjectRecoveryPresenter::startRecoveryFailure() {
  try {
    m_failureView = new RecoveryFailureView(m_mainWindow, this);
    m_failureView->exec();
  } catch (...) {
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

void ProjectRecoveryPresenter::recoverLast() { m_model->recoverLast(); }

void ProjectRecoveryPresenter::openLastInEditor() {
  m_model->openLastInEditor();
}

void ProjectRecoveryPresenter::startMantidNormally() {
  m_model->startMantidNormally();
}

void ProjectRecoveryPresenter::recoverSelectedCheckpoint(QString &selected) {
  std::string stdString = selected.toStdString();
  m_model->recoverSelectedCheckpoint(stdString);
}

void ProjectRecoveryPresenter::openSelectedInEditor(QString &selected) {
  std::string stdString = selected.toStdString();
  m_model->openSelectedInEditor(stdString);
}

void ProjectRecoveryPresenter::closeView() {
  if (m_recView != nullptr) {
    m_recView->setVisible(false);
  }
  if (m_failureView != nullptr) {
    m_failureView->setVisible(false);
  }
}
