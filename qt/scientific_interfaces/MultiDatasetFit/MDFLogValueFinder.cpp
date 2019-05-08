// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MDFLogValueFinder.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include <ostream>

using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace;

namespace MantidQt {
namespace CustomInterfaces {
/**
 * Constructor
 * @param wsNames :: [input] Workspace names
 */
MDFLogValueFinder::MDFLogValueFinder(const QStringList &wsNames)
    : m_wsNames(wsNames) {}

/**
 * Get names of all logs from the first workspace
 * Note:
 *   - No guarantee that other workspaces share these logs
 *   - Names of all logs are returned, not just those convertible to double
 * @returns :: List of log names
 */
std::vector<std::string> MDFLogValueFinder::getLogNames() const {
  std::vector<std::string> logNames;
  if (m_wsNames.empty()) { // no workspaces = no logs
    return logNames;
  }

  auto &ads = AnalysisDataService::Instance();
  const auto &wsName = m_wsNames.first().toStdString();
  if (ads.doesExist(wsName)) {
    const auto &workspace = ads.retrieveWS<MatrixWorkspace>(wsName);
    const auto &logs = workspace->run().getLogData();
    logNames.reserve(logs.size());
    for (const auto &log : logs) {
      logNames.emplace_back(log->name());
    }
  }
  return logNames;
}

/**
 * Get value of log from workspace at given index
 * @param logName :: [input] Name of log
 * @param function :: [input] Function to apply to log e.g. min, max, mean...
 * @param index :: [input] Index of workspace in list
 * @returns :: value of log cast to double
 * @throws std::runtime_error if log cannot be found or cast
 * @throws std::invalid_argument if index is not in range
 */
double MDFLogValueFinder::getLogValue(
    const QString &logName, const Mantid::Kernel::Math::StatisticType &function,
    int index) const {
  if (index > m_wsNames.size() - 1 || index < 0) {
    std::ostringstream message;
    message << "Index " << index
            << " out of range: number of workspaces = " << m_wsNames.size();
    throw std::invalid_argument(message.str());
  }

  return getLogValue(logName, function, m_wsNames.at(index));
}

/**
 * Get value of log from workspace with given name
 * @param logName :: [input] Name of log
 * @param function :: [input] Function to apply to log e.g. min, max, mean...
 * @param wsName :: [input] Name of workspace
 * @returns :: value of log cast to double
 * @throws std::runtime_error if log cannot be found or cast
 * @throws std::invalid_argument if workspace not found
 */
double MDFLogValueFinder::getLogValue(
    const QString &logName, const Mantid::Kernel::Math::StatisticType &function,
    const QString &wsName) const {
  auto &ads = AnalysisDataService::Instance();
  const auto &workspace = wsName.toStdString();
  if (ads.doesExist(workspace)) {
    const auto &ws = ads.retrieveWS<MatrixWorkspace>(workspace);
    return ws->run().getLogAsSingleValue(logName.toStdString(), function);
  } else {
    throw std::invalid_argument("Workspace not found: " + workspace);
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
