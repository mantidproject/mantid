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
  const std::vector<std::string> &getRow(const int i);
  std::vector<std::string> getRow(std::string checkpointName);
  void startMantidNormally();
  void recoverSelectedCheckpoint(std::string &selected);
  void openSelectedInEditor(std::string &selected);
  bool getFailedRun() const;
  bool hasRecoveryStarted() const { return m_recoveryRunning; }
  std::string decideLastCheckpoint();
  void fillRows();
  static int getNumberOfCheckpoints();

private:
  void fillFirstRow();
  void fillRow(const Poco::Path &path, const std::string &checkpointName);
  void updateCheckpointTried(const std::string &checkpointName);
  void createThreadAndManage(const Poco::Path &checkpoint);
  std::vector<std::vector<std::string>> m_rows;
  MantidQt::ProjectRecovery *m_projRec;
  ProjectRecoveryPresenter *m_presenter;
  bool m_failedRun;
  bool m_recoveryRunning;
};

#endif // PROJECTRECOVERYMODEL_H
