#include "MDFLogValueFinder.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"

namespace {
using namespace Mantid::API;

MatrixWorkspace_sptr
getWorkspace(Mantid::API::AnalysisDataServiceImpl const &ads,
             std::string const &name) {
  return ads.retrieveWS<MatrixWorkspace>(name);
}

Run const &getWorkspaceRun(Mantid::API::AnalysisDataServiceImpl const &ads,
                           std::string const &name) {
  return getWorkspace(ads, name)->run();
}

std::vector<Mantid::Kernel::Property *> const &
getWorkspaceLogs(Mantid::API::AnalysisDataServiceImpl const &ads,
                 std::string const &name) {
  return getWorkspaceRun(ads, name).getLogData();
}

std::vector<std::string>
extractLogNames(std::vector<Mantid::Kernel::Property *> const &logs) {
  std::vector<std::string> logNames;
  logNames.reserve(logs.size());
  std::transform(logs.begin(), logs.end(), std::back_inserter(logNames),
                 [](Mantid::Kernel::Property *log) { return log->name(); });
  return logNames;
}

std::vector<std::string> getWorkspaceLogNames(std::string const &name) {
  auto const &ads = AnalysisDataService::Instance();
  if (ads.doesExist(name))
    return extractLogNames(getWorkspaceLogs(ads, name));
  return std::vector<std::string>();
}

double getWorkspaceLogValue(std::string const &logName,
                            Mantid::Kernel::Math::StatisticType const &function,
                            std::string const &workspaceName) {
  auto const &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName))
    return getWorkspaceRun(ads, workspaceName)
        .getLogAsSingleValue(logName, function);
  throw std::invalid_argument(
      "Workspace not found when searching for log value: " + workspaceName);
}

std::string createIndexOutOfRangeMessage(std::size_t index,
                                         std::size_t numberOfWorkspaces) {
  return "Index " + std::to_string(index) +
         " out of range: number of workspaces = " +
         std::to_string(numberOfWorkspaces);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace QENS {

MDFLogValueFinder::MDFLogValueFinder(
    std::vector<std::string> const &workspaceNames)
    : m_workspaceNames(workspaceNames) {}

std::vector<std::string> MDFLogValueFinder::getLogNames() const {
  if (!m_workspaceNames.empty())
    return getWorkspaceLogNames(m_workspaceNames.front());
  return std::vector<std::string>();
}

double MDFLogValueFinder::getLogValue(
    std::string const &logName,
    Mantid::Kernel::Math::StatisticType const &function,
    std::size_t index) const {
  if (index >= m_workspaceNames.size())
    throw std::invalid_argument(
        createIndexOutOfRangeMessage(index, m_workspaceNames.size()));
  return getLogValue(logName, function, m_workspaceNames[index]);
}

double MDFLogValueFinder::getLogValue(
    std::string const &logName,
    Mantid::Kernel::Math::StatisticType const &function,
    std::string const &wsName) const {
  return getWorkspaceLogValue(logName, function, wsName);
}

} // namespace QENS
} // namespace CustomInterfaces
} // namespace MantidQt