#include "ProjectRecoveryModel.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ConfigService.h"
#include "ProjectRecovery.h"
#include "ProjectRecoveryPresenter.h"
#include <Poco/File.h>
#include <Poco/Path.h>
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
}

std::vector<std::string> ProjectRecoveryModel::getRow(int i) {
  return m_rows.at(i);
}

void ProjectRecoveryModel::recoverLast() {
  // Clear the ADS
  Mantid::API::AnalysisDataService::Instance().clear();

  // Recover using the last checkpoint
  Poco::Path output(Mantid::Kernel::ConfigService::Instance().getAppDataDir());
  output.append("ordered_recovery.py");
  auto mostRecentCheckpoints = m_projRec->getRecoveryFolderCheckpointsPR(
      m_projRec->getRecoveryFolderLoadPR());
  auto mostRecentCheckpoint = mostRecentCheckpoints.back();
  m_projRec->openInEditor(mostRecentCheckpoint, output);
  std::thread recoveryThread(
      [=] { m_projRec->loadRecoveryCheckpoint(mostRecentCheckpoint); });
  recoveryThread.detach();

  updateCheckpointTried(
      mostRecentCheckpoint.directory(mostRecentCheckpoint.depth() - 1));

  // Close View
  m_presenter->closeView();
}
void ProjectRecoveryModel::openLastInEditor() {
  // Clear the ADS
  Mantid::API::AnalysisDataService::Instance().clear();

  // Open the last made checkpoint in editor
  auto beforeCheckpoints = m_projRec->getRecoveryFolderLoadPR();
  auto mostRecentCheckpoints =
      m_projRec->getRecoveryFolderCheckpointsPR(beforeCheckpoints);
  auto mostRecentCheckpoint = mostRecentCheckpoints.back();
  Poco::Path output(Mantid::Kernel::ConfigService::Instance().getAppDataDir());
  output.append("ordered_recovery.py");
  m_projRec->openInEditor(mostRecentCheckpoint, output);
  // Restart project recovery as we stay synchronous
  m_projRec->clearAllCheckpoints(beforeCheckpoints);
  m_projRec->startProjectSaving();

  updateCheckpointTried(
      mostRecentCheckpoint.directory(mostRecentCheckpoint.depth() - 1));

  // Close View
  m_presenter->closeView();
}
void ProjectRecoveryModel::startMantidNormally() {
  // Close the window and save if nessercary (Probably not)
  m_projRec->clearAllUnusedCheckpoints();
  m_projRec->startProjectSaving();

  // Close view
  m_presenter->closeView();
}
void ProjectRecoveryModel::recoverSelectedCheckpoint(std::string &selected) {
  // Clear the ADS
  Mantid::API::AnalysisDataService::Instance().clear();

  // Recovery given the checkpoint selected here
  selected.replace(selected.find(" "), 1, "T");
  Poco::Path checkpoint(m_projRec->getRecoveryFolderLoadPR());
  checkpoint.append(selected);
  Poco::Path output(Mantid::Kernel::ConfigService::Instance().getAppDataDir());
  output.append("ordered_recovery.py");

  m_projRec->openInEditor(checkpoint, output);
  std::thread recoveryThread(
      [=] { m_projRec->loadRecoveryCheckpoint(checkpoint); });
  recoveryThread.detach();

  selected.replace(selected.find("T"), 1, " ");
  updateCheckpointTried(selected);

  // Close View
  m_presenter->closeView();
}
void ProjectRecoveryModel::openSelectedInEditor(std::string &selected) {
  // Clear the ADS
  Mantid::API::AnalysisDataService::Instance().clear();

  // Open editor for this checkpoint
  selected.replace(selected.find(" "), 1, "T");
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
  updateCheckpointTried(selected);

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
}

void ProjectRecoveryModel::updateCheckpointTried(
    const std::string &checkpointName) {
  for (auto c : m_rows) {
    if (c[0] == checkpointName) {
      c[3] == "Yes";
    }
  }
}