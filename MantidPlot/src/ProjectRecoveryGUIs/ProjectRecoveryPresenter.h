#ifndef PROJECTRECOVERYPRESENTER_H
#define PROJECTRECOVERYPRESENTER_H

#include "ProjectRecoveryModel.h"
#include <memory>
#include <qt4/QtCore/QStringList>

namespace MantidQt {
class ProjectRecovery;
}
class ProjectRecoveryPresenter {
public:
  // Interestingly this nullptr should never be used
  ProjectRecoveryPresenter(MantidQt::ProjectRecovery *projectRecovery);
  bool startRecoveryView();
  bool startRecoveryFailure();
  QStringList getRow(int i);
  void recoverLast();
  void openLastInEditor();
  void startMantidNormally();
  void recoverSelectedCheckpoint(std::string &selected);
  void openSelectedInEditor(std::string &selected);

private:
  ProjectRecoveryModel m_model;
};

#endif // PROJECTRECOVERYPRESENTER_H
