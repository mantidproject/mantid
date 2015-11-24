#include "MantidQtCustomInterfaces/Reflectometry/TransferResults.h"

namespace MantidQt {
namespace CustomInterfaces {
TransferResults::TransferResults(std::vector<COLUMN_MAP_TYPE> transferRuns,
                                 std::vector<COLUMN_MAP_TYPE> errorRuns) {

  m_transferRuns = transferRuns;
  m_errorRuns = errorRuns;
}

std::vector<std::map<std::string, std::string> > TransferResults::getTransferRuns() {
  return m_transferRuns;
}

std::vector<std::map<std::string, std::string> > TransferResults::getErrorRuns() {
  return m_errorRuns;
}

void TransferResults::addTransferRow(const COLUMN_MAP_TYPE &row) {
  m_transferRuns.push_back(row);
}
void TransferResults::addErrorRow(COLUMN_NAME_TYPE id,
                                  COLUMN_VALUE_TYPE error) {

  COLUMN_MAP_TYPE row;
  std::pair<COLUMN_NAME_TYPE, COLUMN_VALUE_TYPE> pair(id, error);
  row.insert(pair);
  m_errorRuns.push_back(row);
  // m_errorRuns.push_back(row);
}
}
}
