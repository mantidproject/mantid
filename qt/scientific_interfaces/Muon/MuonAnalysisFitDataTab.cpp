//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MuonAnalysisFitDataTab.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MuonAnalysisHelper.h"

#include <boost/shared_ptr.hpp>

//-----------------------------------------------------------------------------

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {

void MuonAnalysisFitDataTab::init() {
  connect(m_uiForm.muonAnalysisHelpDataAnalysis, SIGNAL(clicked()), this,
          SLOT(muonAnalysisHelpDataAnalysisClicked()));

  // Detect when the fit has finished and group the workspaces that have been
  // created as a result.
  connect(m_uiForm.fitBrowser, SIGNAL(fittingDone(QString)), this,
          SLOT(groupFittedWorkspaces(QString)));
}

/**
 * Muon Analysis Data Analysis help (slot)
 */
void MuonAnalysisFitDataTab::muonAnalysisHelpDataAnalysisClicked() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Muon Analysis"), QString("data-analysis"));
}

/**
 * Make a raw workspace by cloning the workspace given which isn't bunched.
 *
 * @param wsName :: The name of the current data (shouldn't be bunched) to
 * clone.
 */
void MuonAnalysisFitDataTab::makeRawWorkspace(const std::string &wsName) {
  Mantid::API::Workspace_sptr inputWs =
      boost::dynamic_pointer_cast<Mantid::API::Workspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(wsName));
  Mantid::API::IAlgorithm_sptr duplicate =
      Mantid::API::AlgorithmManager::Instance().create("CloneWorkspace");
  duplicate->setProperty<Mantid::API::Workspace_sptr>("InputWorkspace",
                                                      inputWs);
  duplicate->setPropertyValue("OutputWorkspace", wsName + "_Raw");
  duplicate->execute();
}

/**
 * Group the fitted workspaces that are created from the 'fit' algorithm
 *
 * @param workspaceName :: The workspaceName that the fit has been done against
 */
void MuonAnalysisFitDataTab::groupFittedWorkspaces(
    const QString &workspaceName) {
  std::string wsNormalised =
      workspaceName.toStdString() + "_NormalisedCovarianceMatrix";
  std::string wsParameters = workspaceName.toStdString() + "_Parameters";
  std::string wsWorkspace = workspaceName.toStdString() + "_Workspace";
  std::vector<std::string> inputWorkspaces;

  if (Mantid::API::AnalysisDataService::Instance().doesExist(wsNormalised)) {
    inputWorkspaces.push_back(wsNormalised);
  }
  if (Mantid::API::AnalysisDataService::Instance().doesExist(wsParameters)) {
    inputWorkspaces.push_back(wsParameters);
  }
  if (Mantid::API::AnalysisDataService::Instance().doesExist(wsWorkspace)) {
    inputWorkspaces.push_back(wsWorkspace);
  }

  if (inputWorkspaces.size() > 1) {
    const std::string groupName = [&workspaceName] {
      const int index = workspaceName.indexOf(';');
      if (index != -1) {
        return workspaceName.left(index).toStdString();
      } else {
        return workspaceName.toStdString();
      }
    }();
    MuonAnalysisHelper::groupWorkspaces(groupName, inputWorkspaces);
  }
}
} // namespace Muon
} // namespace CustomInterfaces
} // namespace MantidQt
