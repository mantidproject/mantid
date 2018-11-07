// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROJECTRECOVERYPRESENTER_H
#define PROJECTRECOVERYPRESENTER_H

#include "ProjectRecoveryModel.h"
#include <QDialog>
#include <QStringList>
#include <boost/shared_ptr.hpp>
#include <memory>

namespace MantidQt {
class ProjectRecovery;
}
class ApplicationWindow;
class ProjectRecoveryView;
class RecoveryFailureView;
class ProjectRecoveryPresenter {
public:
  enum OpenView { RecoveryView, FailureView };
  ProjectRecoveryPresenter(MantidQt::ProjectRecovery *projectRecovery,
                           ApplicationWindow *parentWindow);
  ProjectRecoveryPresenter(const ProjectRecoveryPresenter &obj);
  ~ProjectRecoveryPresenter() = default;
  bool startRecoveryView();
  bool startRecoveryFailure();
  QStringList getRow(int i);
  void recoverLast();
  void openLastInEditor();
  void startMantidNormally();
  void recoverSelectedCheckpoint(const QString &selected);
  void openSelectedInEditor(const QString &selected);
  void closeView();
  void connectProgressBarToRecoveryView();
  ProjectRecoveryPresenter &operator=(const ProjectRecoveryPresenter &obj);
  void emitAbortScript();
  void changeStartMantidToCancelLabel();
  void fillAllRows();
  void setUpProgressBar(const int barMax);
  static int getNumberOfCheckpoints();

private:
  friend class ProjectRecoveryView;
  friend class RecoveryFailureView;
  ApplicationWindow *m_mainWindow;
  std::unique_ptr<ProjectRecoveryView> m_recView;
  std::unique_ptr<RecoveryFailureView> m_failureView;
  std::unique_ptr<ProjectRecoveryModel> m_model;
  OpenView m_openView;
  bool m_startMantidNormallyCalled;
};

#endif // PROJECTRECOVERYPRESENTER_H
