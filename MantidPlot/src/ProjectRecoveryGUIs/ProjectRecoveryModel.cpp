#include "ProjectRecoveryModel.h"
#include "ProjectRecovery.h"
#include <memory>

ProjectRecoveryModel::ProjectRecoveryModel(
    MantidQt::ProjectRecovery *projectRecovery)
    : m_projRec(projectRecovery) {}

std::vector<std::string> ProjectRecoveryModel::getRow(int i) {
  std::vector<std::vector<std::string>> rows = {
      {"2018-09-09 16:04", "77", "Yes"},
      {"2018-09-09 16:03", "76", "No"},
      {"2018-09-09 16:02", "58", "No"},
      {"2018-09-09 16:01", "33", "No"},
      {"2018-09-09 16:00", "22", "No"}};
  return rows.at(i);
}

void ProjectRecoveryModel::recoverLast() {
  // Implement later
}
void ProjectRecoveryModel::openLastInEditor() {
  // Implement later
}
void ProjectRecoveryModel::startMantidNormally() {
  // Implement later
}
void ProjectRecoveryModel::recoverSelectedCheckpoint(std::string &selected) {
  // Implement later
}
void ProjectRecoveryModel::openSelectedInEditor(std::string &selected) {
  // Implement later
}
