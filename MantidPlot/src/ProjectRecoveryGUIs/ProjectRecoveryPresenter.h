// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROJECTRECOVERYPRESENTER_H
#define PROJECTRECOVERYPRESENTER_H

#include "ProjectRecoveryModel.h"
#include <QStringList>
#include <memory>

namespace MantidQt {
class ProjectRecovery;
}
class ApplicationWindow;
class ProjectRecoveryView;
class RecoveryFailureView;
class ProjectRecoveryPresenter {
public:
  // Interestingly this nullptr should never be used
  ProjectRecoveryPresenter(MantidQt::ProjectRecovery *projectRecovery,
                           ApplicationWindow *parentWindow);
  ProjectRecoveryPresenter(const ProjectRecoveryPresenter &obj);
  ~ProjectRecoveryPresenter();
  bool startRecoveryView();
  bool startRecoveryFailure();
  QStringList getRow(int i);
  void recoverLast();
  void openLastInEditor();
  void startMantidNormally();
  void recoverSelectedCheckpoint(QString &selected);
  void openSelectedInEditor(QString &selected);
  void closeView();
  ProjectRecoveryPresenter &operator=(const ProjectRecoveryPresenter &obj);

private:
  ProjectRecoveryModel *m_model;
  ProjectRecoveryView *m_recView;
  RecoveryFailureView *m_failureView;
  ApplicationWindow *m_mainWindow;
};

#endif // PROJECTRECOVERYPRESENTER_H
