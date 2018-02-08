#include "ReflSaveTabPresenter.h"
#include "IReflSaveTabView.h"
#include "IReflMainWindowPresenter.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Run.h"
#include "Poco/File.h"
#include "Poco/Path.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
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
    saveSelectedWorkspaces();
    break;
  case suggestSaveDirFlag:
    suggestSaveDir();
    break;
  case autosaveDisabled:
    disableAutosave();
    break;
  case autosaveEnabled:
    enableAutosave();
    break;
  case autosavePrefixesChanged:
    updateAutosavePrefixes();
    break;
  }
}

namespace {
template <typename InputIterator>
bool startsWithAnyOf(InputIterator begin, InputIterator end,
                     std::string const &name) {
  for (auto current = begin; current != end; ++current)
    if (boost::algorithm::starts_with(name, *current))
      return true;
  return false;
}
}

std::vector<std::string>
ReflSaveTabPresenter::parseAutosavePrefixes(std::string const &prefixesInput) {
  auto prefixes = std::vector<std::string>();
  if (!prefixesInput.empty()) {
    boost::split(prefixes, prefixesInput, boost::is_any_of(","));
    return prefixes;
  } else {
    return prefixes;
  }
}

void ReflSaveTabPresenter::enableAutosave() {
  if (isValidSaveDirectory(m_view->getSavePath()))
    updateAutosavePrefixes();
  else {
    warnInvalidSaveDirectory();
    m_view->disallowAutosave();
  }
}

void ReflSaveTabPresenter::disableAutosave() {
  m_autosavePrefixWhiteList = boost::none;
}

void ReflSaveTabPresenter::updateAutosavePrefixes() {
  m_autosavePrefixWhiteList =
      parseAutosavePrefixes(m_view->getAutosavePrefixInput());
}

bool ReflSaveTabPresenter::isWhitelisted(
    std::vector<std::string> const &prefixWhitelist,
    std::string const &workspaceName) const {
  return startsWithAnyOf(prefixWhitelist.cbegin(), prefixWhitelist.cend(),
                         workspaceName);
}

std::vector<std::string> ReflSaveTabPresenter::filterByWhitelist(
    std::vector<std::string> const &prefixWhitelist,
    std::vector<std::string> workspaceNames) const {
  workspaceNames.erase(
      std::remove_if(
          workspaceNames.begin(), workspaceNames.end(),
          [this, &prefixWhitelist](std::string const &workspaceName) -> bool {
            return !isWhitelisted(prefixWhitelist, workspaceName);
          }),
      workspaceNames.end());
  return workspaceNames;
}

void ReflSaveTabPresenter::autoSaveWorkspaces(
    std::vector<std::string> const &prefixWhitelist,
    std::vector<std::string> const &workspaceNames) {
  auto workspacesToSave = filterByWhitelist(prefixWhitelist, workspaceNames);
  if (!workspacesToSave.empty())
    saveWorkspaces(workspaceNames, std::vector<std::string>());

  //std::cout << "** Begin Saving" << std::endl;
  //for (auto &&ws : workspacesToSave)
  //  std::cout << "** Saving " << ws << std::endl;
  //std::cout << "** End Saving" << std::endl;
}

void ReflSaveTabPresenter::completedGroupReductionSuccessfully(
    MantidWidgets::DataProcessor::GroupData const &group,
    std::string const &workspaceName) {
  if (m_autosavePrefixWhiteList)
    autoSaveWorkspaces(m_autosavePrefixWhiteList.get(),
                       std::vector<std::string>({workspaceName}));
}

void ReflSaveTabPresenter::completedRowReductionSuccessfully(
    MantidWidgets::DataProcessor::GroupData const &group,
    std::vector<std::string> const &workspaceNames) {
  if (!MantidWidgets::DataProcessor::canPostprocess(group) &&
      m_autosavePrefixWhiteList)
    autoSaveWorkspaces(m_autosavePrefixWhiteList.get(), workspaceNames);
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

bool ReflSaveTabPresenter::isValidSaveDirectory(std::string const &directory) {
  auto file = Poco::File(directory);
  return !directory.empty() && file.exists() && file.isDirectory();
}

void ReflSaveTabPresenter::warn(std::string const &message,
                                std::string const &title) {
  m_mainPresenter->giveUserCritical(message, title);
}

std::vector<std::string>
titles(std::vector<std::string> const &workspaceNames) {
  auto workspaceTitles = std::vector<std::string>();
  workspaceTitles.reserve(workspaceNames.size());
  std::transform(workspaceNames.begin(), workspaceNames.end(),
                 std::back_inserter(workspaceTitles), [](std::string s) {
                   return AnalysisDataService::Instance()
                       .retrieveWS<MatrixWorkspace>(s)
                       ->getTitle();
                 });
  return workspaceTitles;
}

void ReflSaveTabPresenter::warnInvalidSaveDirectory() {
  warn("Directory specified doesn't exist or "
       "was invalid for your operating system",
       "Invalid directory");
}

void ReflSaveTabPresenter::saveWorkspaces(
    std::vector<std::string> const &workspaceNames,
    std::vector<std::string> const &logParameters) {
  if (isValidSaveDirectory(m_view->getSavePath())) {
    auto workspaceTitles = titles(workspaceNames);

    // Create the appropriate save algorithm
    bool titleCheck = m_view->getTitleCheck();
    bool qResolutionCheck = m_view->getQResolutionCheck();
    std::string separator = m_view->getSeparator();
    std::string prefix = m_view->getPrefix();
    int formatIndex = m_view->getFileFormatIndex();
    std::string algName = m_saveAlgs[formatIndex];
    std::string extension = m_saveExts[formatIndex];
    IAlgorithm_sptr saveAlg = AlgorithmManager::Instance().create(algName);
    auto saveDir = m_view->getSavePath();

    auto titleIt = workspaceTitles.cbegin();
    auto nameIt = workspaceNames.cbegin();
    for (; titleIt != workspaceTitles.cend() && nameIt != workspaceNames.cend();
         ++titleIt, ++nameIt) {
      auto &name = *nameIt;
      auto &title = *titleIt;
      // Add any additional algorithm-specific properties and execute
      if (algName != "SaveANSTOAscii") {
        if (titleCheck)
          saveAlg->setProperty("Title", title);
        saveAlg->setProperty("LogList", logParameters);
      }
      if (algName == "SaveReflCustomAscii") {
        saveAlg->setProperty("WriteDeltaQ", qResolutionCheck);
      }

      auto path = Poco::Path(saveDir);
      path.append(prefix + name + extension);
      saveAlg->setProperty("Separator", separator);
      saveAlg->setProperty("Filename", path.toString());
      saveAlg->setProperty(
          "InputWorkspace",
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name));
      saveAlg->execute();
    }
  } else {
    warnInvalidSaveDirectory();
  }
}

/** Saves workspaces with the names specified. */
void ReflSaveTabPresenter::saveWorkspaces(
    std::vector<std::string> const &workspaceNames) {
  auto selectedLogParameters = m_view->getSelectedParameters();
  saveWorkspaces(workspaceNames, selectedLogParameters);
}

/** Saves selected workspaces */
void ReflSaveTabPresenter::saveSelectedWorkspaces() {
  // Check that at least one workspace has been selected for saving
  auto workspaceNames = m_view->getSelectedWorkspaces();
  if (workspaceNames.empty()) {
    m_mainPresenter->giveUserCritical("No workspaces selected. You must select "
                                      "the workspaces to save.",
                                      "No workspaces selected");
  } else {
    saveWorkspaces(workspaceNames);
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
