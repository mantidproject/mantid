#include "MantidQtCustomInterfaces/Reflectometry/TransferResults.h"

namespace MantidQt {
namespace CustomInterfaces {
TransferResults::TransferResults(
    std::vector<std::map<std::string, std::string>> transferRuns,
    std::vector<std::map<std::string, std::string>> errorRuns) {

  m_transferRuns = transferRuns;
  m_errorRuns = errorRuns;
}

std::vector<std::map<std::string, std::string>>
TransferResults::getTransferRuns() {
  return m_transferRuns;
}
std::vector<std::map<std::string, std::string>>
TransferResults::getErrorRuns() {
  return m_errorRuns;
}

void TransferResults::addTransferRow(
    const std::map<std::string, std::string> &row) {
  m_transferRuns.push_back(row);
}
void TransferResults::addErrorRow(std::string id, std::string error) {
  
  std::map<std::string, std::string> row;
  std::pair<std::string, std::string> pair(id, error);
  row.insert(pair);
  m_errorRuns.push_back(row);
  // m_errorRuns.push_back(row);
}
}
}
