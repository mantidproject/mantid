#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowPresenter.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Run.h"
#include "Poco/File.h"
#include "Poco/Path.h"

#include <boost/regex.hpp>

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;

/** Constructor
* @param view :: The view we are handling
*/
ReflSaveTabPresenter::ReflSaveTabPresenter(IReflSaveTabView *view)
    : m_view(view), m_mainPresenter() {

  m_saveAlgs = {"SaveReflCustomAscii", "SaveReflThreeColumnAscii",
                "SaveANSTOAscii", "SaveILLCosmosAscii"};
  m_saveExts = {".dat", ".dat", ".txt", ".mft"};
}

/** Destructor
*/
ReflSaveTabPresenter::~ReflSaveTabPresenter() {}

/** Accept a main presenter
* @param mainPresenter :: [input] The main presenter
*/
void ReflSaveTabPresenter::acceptMainPresenter(
    IReflMainWindowPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

void ReflSaveTabPresenter::notify(IReflSaveTabPresenter::Flag flag) {
  switch (flag) {
  case populateWorkspaceListFlag:
    populateWorkspaceList();
    break;
  case filterWorkspaceListFlag:
    filterWorkspaceNames();
    break;
  case workspaceParamsFlag:
    populateParametersList();
    break;
  case saveWorkspacesFlag:
    saveWorkspaces();
    break;
  case suggestSaveDirFlag:
    suggestSaveDir();
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
*/
void ReflSaveTabPresenter::filterWorkspaceNames() {
  m_view->clearWorkspaceList();

  std::string filter = m_view->getFilter();
  bool regexCheck = m_view->getRegexCheck();
  auto wsNames = getAvailableWorkspaceNames();
  std::vector<std::string> validNames(wsNames.size());
  auto it = validNames.begin();

  if (regexCheck) {
    // Use regex search to find names that contain the filter sequence
    try {
      boost::regex rgx(filter);
      it = std::copy_if(
          wsNames.begin(), wsNames.end(), validNames.begin(),
          [rgx](std::string s) { return boost::regex_search(s, rgx); });
    } catch (boost::regex_error &) {
      m_mainPresenter->giveUserCritical("Error, invalid regular expression\n",
                                        "Invalid regex");
    }
  } else {
    // Otherwise simply add names where the filter string is found in
    it = std::copy_if(wsNames.begin(), wsNames.end(), validNames.begin(),
                      [filter](std::string s) {
                        return s.find(filter) != std::string::npos;
                      });
  }

  validNames.resize(std::distance(validNames.begin(), it));
  m_view->setWorkspaceList(validNames);
}

/** Fills the 'List of Logged Parameters' widget with the parameters of the
* currently selected workspace
*/
void ReflSaveTabPresenter::populateParametersList() {
  m_view->clearParametersList();

  std::string wsName = m_view->getCurrentWorkspaceName();
  std::vector<std::string> logs;
  const auto &properties = AnalysisDataService::Instance()
                               .retrieveWS<MatrixWorkspace>(wsName)
                               ->run()
                               .getProperties();
  for (auto it = properties.begin(); it != properties.end(); it++) {
    logs.push_back((*it)->name());
  }
  m_view->setParametersList(logs);
}

/** Saves selected workspaces
*/
void ReflSaveTabPresenter::saveWorkspaces() {
  // Check that save directory is valid
  std::string saveDir = m_view->getSavePath();
  if (saveDir.empty() || Poco::File(saveDir).isDirectory() == false) {
    m_mainPresenter->giveUserCritical("Directory specified doesn't exist or "
                                      "was invalid for your operating system",
                                      "Invalid directory");
    return;
  }

  // Check that at least one workspace has been selected for saving
  auto wsNames = m_view->getSelectedWorkspaces();
  if (wsNames.empty()) {
    m_mainPresenter->giveUserCritical("No workspaces selected. You must select "
                                      "the workspaces to save.",
                                      "No workspaces selected");
  }

  // Obtain workspace titles
  std::vector<std::string> wsTitles(wsNames.size());
  std::transform(wsNames.begin(), wsNames.end(), wsTitles.begin(),
                 [](std::string s) {
                   return AnalysisDataService::Instance()
                       .retrieveWS<MatrixWorkspace>(s)
                       ->getTitle();
                 });

  // Create the appropriate save algorithm
  bool titleCheck = m_view->getTitleCheck();
  auto selectedParameters = m_view->getSelectedParameters();
  bool qResolutionCheck = m_view->getQResolutionCheck();
  std::string separator = m_view->getSeparator();
  std::string prefix = m_view->getPrefix();
  int formatIndex = m_view->getFileFormatIndex();
  std::string algName = m_saveAlgs[formatIndex];
  std::string extension = m_saveExts[formatIndex];
  IAlgorithm_sptr saveAlg = AlgorithmManager::Instance().create(algName);

  for (size_t i = 0; i < wsNames.size(); i++) {
    // Add any additional algorithm-specific properties and execute
    if (algName != "SaveANSTOAscii") {
      if (titleCheck)
        saveAlg->setProperty("Title", wsTitles[i]);
      saveAlg->setProperty("LogList", selectedParameters);
    }
    if (algName == "SaveReflCustomAscii") {
      saveAlg->setProperty("WriteDeltaQ", qResolutionCheck);
    }

    auto path = Poco::Path(saveDir);
    auto wsName = wsNames[i];
    path.append(prefix + wsName + extension);
    saveAlg->setProperty("Separator", separator);
    saveAlg->setProperty("Filename", path.toString());
    saveAlg->setProperty(
        "InputWorkspace",
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName));
    saveAlg->execute();
  }
}

/** Suggests a save directory and sets it in the 'Save path' text field
*/
void ReflSaveTabPresenter::suggestSaveDir() {
  std::string path = Mantid::Kernel::ConfigService::Instance().getString(
      "defaultsave.directory");
  m_view->setSavePath(path);
}

/** Obtains all available workspace names to save
* @return :: list of workspace names
*/
std::vector<std::string> ReflSaveTabPresenter::getAvailableWorkspaceNames() {
  auto allNames = AnalysisDataService::Instance().getObjectNames();
  // Exclude workspace groups and table workspaces as they cannot be saved to
  // ascii
  std::vector<std::string> validNames;
  std::remove_copy_if(
      allNames.begin(), allNames.end(), std::back_inserter(validNames),
      [](const std::string &wsName) {
        return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
                   wsName) ||
               AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
                   wsName);
      });

  return validNames;
}
}
}