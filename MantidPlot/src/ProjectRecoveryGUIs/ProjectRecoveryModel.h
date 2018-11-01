// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROJECTRECOVERYMODEL_H
#define PROJECTRECOVERYMODEL_H

#include <Poco/Path.h>
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
  std::vector<std::string> getRow(const std::string &checkpointName);
  void startMantidNormally();
  void recoverSelectedCheckpoint(std::string &selected);
  void openSelectedInEditor(std::string &selected);
  bool getFailedRun() const;
  std::string decideLastCheckpoint();

private:
  void fillRows();
  void updateCheckpointTried(const std::string &checkpointName);
  bool checkRecoverWasASuccess(const std::string &projectFile);
  void createThreadAndManage(const Poco::Path &checkpoint);
  std::vector<std::vector<std::string>> m_rows;
  MantidQt::ProjectRecovery *m_projRec;
  ProjectRecoveryPresenter *m_presenter;
  bool m_failedRun;
};

#endif // PROJECTRECOVERYMODEL_H
