// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ProjectRecoveryModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ConfigService.h"
#include "ProjectRecovery.h"
#include "ProjectRecoveryPresenter.h"
#include "RecoveryThread.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <QApplication>
#include <QThread>
#include <fstream>
#include <memory>

namespace {
std::string findNumberOfWorkspacesInDirectory(const Poco::Path &path) {
  std::vector<std::string> files;
  Poco::File(path).list(files);
  // Number of workspaces is equal to the number of files in the path directory
  // -1 of that value.
  return std::to_string(files.size() - 1);
}
void replaceSpaceWithT(std::string &string) {
  const auto stringSpacePos = string.find(" ");
  if (stringSpacePos != std::string::npos)
    string.replace(stringSpacePos, 1, "T");
}
void sortByLastModified(std::vector<Poco::Path> &paths) {
  std::sort(paths.begin(), paths.end(), [](const auto &a, const auto &b) {
    Poco::File a1(a);
    Poco::File b1(b); // Last modified is first!
    return a1.getLastModified() > b1.getLastModified();
  });
}
} // namespace

ProjectRecoveryModel::ProjectRecoveryModel(
    MantidQt::ProjectRecovery *projectRecovery,
    ProjectRecoveryPresenter *presenter)
    : m_projRec(projectRecovery), m_presenter(presenter), m_failedRun(true),
      m_recoveryRunning(false) {
  fillFirstRow();
}

const std::vector<std::string> &ProjectRecoveryModel::getRow(const int i) {
  return m_rows.at(i);
}

std::vector<std::string>
ProjectRecoveryModel::getRow(std::string checkpointName) {
  replaceSpaceWithT(checkpointName);
  for (auto c : m_rows) {
    if (c[0] == checkpointName) {
      return c;
    }
  }
  return std::vector<std::string>({"", "", "0"});
}

void ProjectRecoveryModel::startMantidNormally() {
  m_projRec->clearAllUnusedCheckpoints();
  m_projRec->startProjectSaving();
  m_failedRun = false;

  // If project recovery is running on script window we need to abort
  if (m_recoveryRunning) {
    m_presenter->emitAbortScript();
  }

  // Close view
  m_presenter->closeView();
}
void ProjectRecoveryModel::recoverSelectedCheckpoint(std::string &selected) {
  m_recoveryRunning = true;
  m_presenter->changeStartMantidToCancelLabel();
  // Clear the ADS
  Mantid::API::AnalysisDataService::Instance().clear();

  // Recovery given the checkpoint selected here
  replaceSpaceWithT(selected);
  Poco::Path checkpoint(m_projRec->getRecoveryFolderLoadPR());
  checkpoint.append(selected);
  Poco::Path output(Mantid::Kernel::ConfigService::Instance().getAppDataDir());
  output.append("ordered_recovery.py");

  m_projRec->openInEditor(checkpoint, output);
  createThreadAndManage(checkpoint);

  selected.replace(selected.find("T"), 1, " ");
  if (m_failedRun) {
    updateCheckpointTried(selected);
  }
  m_recoveryRunning = false;
  // Close View
  m_presenter->closeView();
}
void ProjectRecoveryModel::openSelectedInEditor(std::string &selected) {
  m_recoveryRunning = true;
  // Clear the ADS
  Mantid::API::AnalysisDataService::Instance().clear();

  // Open editor for this checkpoint
  replaceSpaceWithT(selected);
  auto beforeCheckpoint = m_projRec->getRecoveryFolderLoadPR();
  Poco::Path checkpoint(beforeCheckpoint);
  checkpoint.append(selected);
  Poco::Path output(Mantid::Kernel::ConfigService::Instance().getAppDataDir());
  output.append("ordered_recovery.py");

  m_projRec->openInEditor(checkpoint, output);
  // Restart project recovery as we stay synchronous
  m_projRec->clearAllCheckpoints(beforeCheckpoint);
  m_projRec->startProjectSaving();

  selected.replace(selected.find("T"), 1, " ");
  if (m_failedRun) {
    updateCheckpointTried(selected);
  }
  m_recoveryRunning = false;
  m_failedRun = false;
  // Close View
  m_presenter->closeView();
}

void ProjectRecoveryModel::fillRow(const Poco::Path &path,
                                   const std::string &checkpointName) {
  std::string lengthOfFile = findNumberOfWorkspacesInDirectory(path);
  std::string checked = "No";
  std::vector<std::string> nextVector = {
      std::move(checkpointName), std::move(lengthOfFile), std::move(checked)};
  m_rows.emplace_back(std::move(nextVector));
}

void ProjectRecoveryModel::fillFirstRow() {
  auto paths = m_projRec->getListOfFoldersInDirectoryPR(
      m_projRec->getRecoveryFolderLoadPR());
  sortByLastModified(paths);

  // Grab the first path as that is the one that should be loaded
  const auto path = paths.front();
  std::string checkpointName = path.directory(path.depth() - 1);
  checkpointName.replace(checkpointName.find("T"), 1, " ");
  fillRow(path, checkpointName);
}

void ProjectRecoveryModel::fillRows() {
  auto paths = m_projRec->getListOfFoldersInDirectoryPR(
      m_projRec->getRecoveryFolderLoadPR());
  sortByLastModified(paths);

  // Sort the rows first string of the vector lists
  for (const auto &c : paths) {
    std::string checkpointName = c.directory(c.depth() - 1);
    checkpointName.replace(checkpointName.find("T"), 1, " ");
    // Check if there is a first row already and skip first one
    if (m_rows[0][0] == checkpointName)
      continue;

    fillRow(c, checkpointName);
  }

  // Get the number of checkpoints from ConfigService
  int numberOfCheckpoints = getNumberOfCheckpoints();
  for (auto i = paths.size();
       i < static_cast<unsigned int>(numberOfCheckpoints); ++i) {
    std::vector<std::string> newVector = {"", "", ""};
    m_rows.emplace_back(std::move(newVector));
  }

  // order the vector based on save date and time Most recent first
  std::sort(m_rows.begin(), m_rows.end(),
            [](const auto &a, const auto &b) -> bool {
              return a.front() > b.front();
            });
}

void ProjectRecoveryModel::updateCheckpointTried(
    const std::string &checkpointName) {
  for (auto &c : m_rows) {
    if (c[0] == checkpointName) {
      c[2] = "Yes";
      return;
    }
  }
  throw std::runtime_error("Passed checkpoint name for update was incorrect: " +
                           checkpointName);
}

bool ProjectRecoveryModel::getFailedRun() const { return m_failedRun; }

void ProjectRecoveryModel::createThreadAndManage(const Poco::Path &checkpoint) {
  RecoveryThread recoverThread;
  recoverThread.setProjRecPtr(m_projRec);
  recoverThread.setCheckpoint(checkpoint);
  recoverThread.start(QThread::LowPriority);

  while (!recoverThread.isFinished()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    QApplication::processEvents();
  }

  // Set failed run member to the value from the thread
  m_failedRun = recoverThread.getFailedRun();
}

std::string ProjectRecoveryModel::decideLastCheckpoint() {
  auto mostRecentCheckpoints = m_projRec->getRecoveryFolderCheckpointsPR(
      m_projRec->getRecoveryFolderLoadPR());
  auto mostRecentCheckpointPath = mostRecentCheckpoints.back();
  return mostRecentCheckpointPath.directory(mostRecentCheckpointPath.depth() -
                                            1);
}

int ProjectRecoveryModel::getNumberOfCheckpoints() {
  int numberOfCheckpoints;
  try {
    numberOfCheckpoints =
        std::stoi(Mantid::Kernel::ConfigService::Instance().getString(
            "projectRecovery.numberOfCheckpoints"));
  } catch (...) {
    // Fail silently and set to 5
    numberOfCheckpoints = 5;
  }
  return numberOfCheckpoints;
}