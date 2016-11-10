#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabView.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"

#include <regex>

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;

Mantid::Kernel::Logger ReflSaveTabPresenter::g_log("ReflSaveTabPresenter");

/** Constructor
* @param view :: The view we are handling
*/
ReflSaveTabPresenter::ReflSaveTabPresenter(IReflSaveTabView *view)
    : m_view(view) {

  FrameworkManager::Instance();
}

/** Destructor
*/
ReflSaveTabPresenter::~ReflSaveTabPresenter() {}

void ReflSaveTabPresenter::notify(IReflSaveTabPresenter::Flag flag) {
  switch (flag) {
  case populateWorkspaceListFlag:
    populateWorkspaceList();
    break;
  case filterWorkspaceListFlag:
    filterWorkspaceNames(m_view->getFilter(), m_view->getRegexCheck());
    break;
  case workspaceParamsFlag:
    populateParametersList(m_view->getCurrentWorkspaceName());
    break;
  }
}

/** Fills the 'List of Workspaces' widget with the names of all available
* workspaces
*/
void ReflSaveTabPresenter::populateWorkspaceList() {
  m_view->clearWorkspaceList();
  m_view->setWorkspaceList(getAvailableWorkspaceNames());
}

/** Filters the names in the 'List of Workspaces' widget
* @param filter :: filter text string
* @param regexCheck :: whether to use regex in filtering
*/
void ReflSaveTabPresenter::filterWorkspaceNames(std::string filter, 
    bool regexCheck) {
  m_view->clearWorkspaceList();
  auto wsNames = getAvailableWorkspaceNames();
  std::vector<std::string> validNames(wsNames.size());
  auto it = validNames.begin();

  if (regexCheck) {
    // Use regex search to find names that contain the filter sequence
    try {
      std::regex rgx(filter);
      it = std::copy_if(wsNames.begin(), wsNames.end(), validNames.begin(),
        [rgx](std::string s) { return std::regex_search(s, rgx); });
    } catch (std::regex_error&) {
      g_log.error("Error, invalid regular expression\n");
    }
  } else {
    // Otherwise simply add names where the filter string is found in
    it = std::copy_if(wsNames.begin(), wsNames.end(), validNames.begin(),
      [filter](std::string s) { return s.find(filter) != std::string::npos; });
  }

  validNames.resize(std::distance(validNames.begin(), it));
  m_view->setWorkspaceList(validNames);
}

/** Fills the 'List of Logged Parameters' widget with the parameters of the
* currently selected workspace
* @param wsName :: name of selected workspace
*/
void ReflSaveTabPresenter::populateParametersList(std::string wsName) {
  m_view->clearParametersList();

  std::vector<std::string> logs;
  const auto &properties = AnalysisDataService::Instance().retrieveWS
    <MatrixWorkspace>(wsName)->run().getProperties();
  for (auto it = properties.begin(); it != properties.end(); it++) {
    logs.push_back((*it)->name());
  }
  m_view->setParametersList(logs);
}

/** Obtains all available workspace names to save
* @return :: list of workspace names
*/
std::vector<std::string> ReflSaveTabPresenter::getAvailableWorkspaceNames() {
  auto allNames = AnalysisDataService::Instance().getObjectNames();
  // Exclude workspace groups as they cannot be saved to ascii
  std::vector<std::string> validNames(allNames.size());
  auto it = std::copy_if(allNames.begin(), allNames.end(), validNames.begin(),
    [](std::string s) { return (
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(s) == false); 
  });
  validNames.resize(std::distance(validNames.begin(), it));

  return validNames;
}
}
}