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

private:
  ProjectRecoveryModel *m_model;
  ProjectRecoveryView *m_recView;
  RecoveryFailureView *m_failureView;
  ApplicationWindow *m_mainWindow;
};

#endif // PROJECTRECOVERYPRESENTER_H
