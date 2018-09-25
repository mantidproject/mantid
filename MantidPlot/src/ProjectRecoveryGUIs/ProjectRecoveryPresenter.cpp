#include "ProjectRecoveryPresenter.h"
#include "ProjectRecoveryModel.h"
#include "ProjectRecoveryView.h"
#include "RecoveryFailureView.h"
#include "ProjectRecovery.h"

#include <memory>

ProjectRecoveryPresenter::ProjectRecoveryPresenter(
    MantidQt::ProjectRecovery *projectRecovery)
    : m_model(projectRecovery) {}

bool ProjectRecoveryPresenter::startRecoveryView() {
  try {
    ProjectRecoveryView w(0, this);
    w.show();
  } catch (...) {
    return true;
  }
  return false;
}

bool ProjectRecoveryPresenter::startRecoveryFailure() {
  try {
    RecoveryFailureView w(0, this);
    w.show();
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
