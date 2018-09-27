#include "ProjectRecoveryPresenter.h"
#include "ProjectRecovery.h"
#include "ProjectRecoveryModel.h"
#include "ProjectRecoveryView.h"
#include "RecoveryFailureView.h"
#include "ApplicationWindow.h"

#include <memory>

ProjectRecoveryPresenter::ProjectRecoveryPresenter(
    MantidQt::ProjectRecovery *projectRecovery, ApplicationWindow *parentWindow)
    : m_model(projectRecovery, this), m_mainWindow(parentWindow) {
      m_recView = nullptr;
      m_failureView = nullptr;
    }

ProjectRecoveryPresenter::~ProjectRecoveryPresenter(){
  delete m_recView;
  delete m_failureView;
}

bool ProjectRecoveryPresenter::startRecoveryView() {
  try {
    m_recView = new ProjectRecoveryView(m_mainWindow, this);
    m_recView->show();
  } catch (...) {
    return true;
  }
  return false;
}

bool ProjectRecoveryPresenter::startRecoveryFailure() {
  try {
    m_failureView = new RecoveryFailureView(m_mainWindow, this);
    m_failureView->show();
  } catch (...) {
    return true;
  }
  return false;
}

QStringList ProjectRecoveryPresenter::getRow(int i) {
  auto vec = m_model.getRow(i);
  QStringList returnVal;
  for (auto i = 0; i < 3; ++i) {
    QString newString = QString::fromStdString(vec[i]);
    returnVal << newString;
  }
  return returnVal;
}

void ProjectRecoveryPresenter::recoverLast() { m_model.recoverLast(); }

void ProjectRecoveryPresenter::openLastInEditor() {
  m_model.openLastInEditor();
}

void ProjectRecoveryPresenter::startMantidNormally() {
  m_model.startMantidNormally();
}

void ProjectRecoveryPresenter::recoverSelectedCheckpoint(
    std::string &selected) {
  m_model.recoverSelectedCheckpoint(selected);
}

void ProjectRecoveryPresenter::openSelectedInEditor(std::string &selected) {
  m_model.openSelectedInEditor(selected);
}

void ProjectRecoveryPresenter::closeView(){
  if (m_recView != nullptr){
    m_recView->setVisible(false);
  }
  if (m_failureView != nullptr){
    m_failureView->setVisible(false);
  }
}
