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
std::string lengthOfRecoveryFile(Poco::Path path) {
  Poco::Path output(Mantid::Kernel::ConfigService::Instance().getAppDataDir());
  output.append("ordered_temp.py");
  const std::string algName = "OrderWorkspaceHistory";
  auto alg =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged(algName, 1);
  alg->initialize();
  alg->setChild(true);
  alg->setLogging(false);
  alg->setRethrows(true);
  alg->setProperty("RecoveryCheckpointFolder", path.toString());
  alg->setProperty("OutputFilepath", output.toString());
  alg->execute();

  std::ifstream fileCount(output.toString());
  auto lineLength = std::count(std::istreambuf_iterator<char>(fileCount),
                               std::istreambuf_iterator<char>(), '\n');

  // Clean up this length creation
  fileCount.close();
  Poco::File file(output);
  file.remove();

  return std::to_string(lineLength);
}
} // namespace

ProjectRecoveryModel::ProjectRecoveryModel(
    MantidQt::ProjectRecovery *projectRecovery,
    ProjectRecoveryPresenter *presenter)
    : m_projRec(projectRecovery), m_presenter(presenter) {
  fillRows();
  // Default to failed run
  m_failedRun = true;
}

std::vector<std::string> ProjectRecoveryModel::getRow(int i) {
  return m_rows.at(i);
}

std::vector<std::string>
ProjectRecoveryModel::getRow(std::string checkpointName) {
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
  // Close view
  m_presenter->closeView();
}
void ProjectRecoveryModel::recoverSelectedCheckpoint(std::string &selected) {
  // Clear the ADS
  Mantid::API::AnalysisDataService::Instance().clear();

  // Recovery given the checkpoint selected here
  auto stringSpacePos = selected.find(" ");
  if (stringSpacePos != std::string::npos)
    selected.replace(stringSpacePos, 1, "T");
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

  // Close View
  m_presenter->closeView();
}
void ProjectRecoveryModel::openSelectedInEditor(std::string &selected) {
  // Clear the ADS
  Mantid::API::AnalysisDataService::Instance().clear();

  // Open editor for this checkpoint
  auto stringSpacePos = selected.find(" ");
  if (stringSpacePos != std::string::npos)
    selected.replace(stringSpacePos, 1, "T");
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

  m_failedRun = false;
  // Close View
  m_presenter->closeView();
}

void ProjectRecoveryModel::fillRows() {
  auto paths = m_projRec->getListOfFoldersInDirectoryPR(
      m_projRec->getRecoveryFolderLoadPR());

  // Sort the rows first string of the vector lists
  for (auto c : paths) {
    std::string checkpointName = c.directory(c.depth() - 1);
    checkpointName.replace(checkpointName.find("T"), 1, " ");
    std::string lengthOfFile = lengthOfRecoveryFile(c);
    std::string checked = "No";
    std::vector<std::string> nextVector = {checkpointName, lengthOfFile,
                                           checked};
    m_rows.emplace_back(nextVector);
  }

  for (auto i = paths.size(); i < 5; ++i) {
    std::vector<std::string> newVector = {"", "", ""};
    m_rows.emplace_back(newVector);
  }

  // order the vector based on save date and time Most recent first
  std::sort(m_rows.begin(), m_rows.end(),
            [](auto &a, auto &b) -> bool { return a[0] > b[0]; });
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