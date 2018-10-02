#ifndef PROJECTRECOVERYMODEL_H
#define PROJECTRECOVERYMODEL_H

#include <memory>
#include <string>
#include <vector>
namespace MantidQt {
class ProjectRecovery;
}
class ProjectRecoveryPresenter;
class ProjectRecoveryModel {
public:
  ProjectRecoveryModel(MantidQt::ProjectRecovery *projectRecovery,
                       ProjectRecoveryPresenter *presenter);
  std::vector<std::string> getRow(int i);
  void recoverLast();
  void openLastInEditor();
  void startMantidNormally();
  void recoverSelectedCheckpoint(std::string &selected);
  void openSelectedInEditor(std::string &selected);

private:
  void fillRows();
  void updateCheckpointTried(const std::string &checkpointName);
  std::vector<std::vector<std::string>> m_rows;
  MantidQt::ProjectRecovery *m_projRec;
  ProjectRecoveryPresenter *m_presenter;
};

#endif // PROJECTRECOVERYMODEL_H
