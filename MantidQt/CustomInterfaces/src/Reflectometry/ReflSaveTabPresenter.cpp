#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabView.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid::API;

/** Constructor
* @param view :: The view we are handling
*/
ReflSaveTabPresenter::ReflSaveTabPresenter(IReflSaveTabView *view)
    : m_view(view) {
}

/** Destructor
*/
ReflSaveTabPresenter::~ReflSaveTabPresenter() {}

void ReflSaveTabPresenter::notify(IReflSaveTabPresenter::Flag flag) {
  switch (flag) {
  case populateWorkspaceListFlag:
    populateWorkspaceList();
  }
}

/** Fills the 'List of Workspaces' text field with the names of all available
* workspaces
*/
void ReflSaveTabPresenter::populateWorkspaceList() {
  m_view->clearWorkspaceList();
  m_view->setWorkspaceList(getAvailableWorkspaceNames());
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