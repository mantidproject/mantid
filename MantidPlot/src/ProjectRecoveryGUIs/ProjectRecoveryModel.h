#ifndef PROJECTRECOVERYMODEL_H
#define PROJECTRECOVERYMODEL_H

#include <memory>
#include <string>
#include <vector>
namespace MantidQt {
class ProjectRecovery;
}
class ProjectRecoveryModel {
public:
  ProjectRecoveryModel(MantidQt::ProjectRecovery *projectRecovery);
  std::vector<std::string> getRow(int i);
  void recoverLast();
  void openLastInEditor();
  void startMantidNormally();
  void recoverSelectedCheckpoint(std::string &selected);
  void openSelectedInEditor(std::string &selected);

private:
  MantidQt::ProjectRecovery* m_projRec;
};

#endif // PROJECTRECOVERYMODEL_H
