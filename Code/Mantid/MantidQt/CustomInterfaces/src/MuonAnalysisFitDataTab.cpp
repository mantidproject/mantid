//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysisFitDataTab.h"
#include "MantidKernel/ConfigService.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"

#include <boost/shared_ptr.hpp>
#include <fstream>

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QTemporaryFile>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QtBoolPropertyManager>

//-----------------------------------------------------------------------------

namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{


/**
* Make a raw workspace by cloning the workspace given which isn't bunched.
*
* @params wsName :: The name of the current data (shouldn't be bunched) to clone.
*/
void MuonAnalysisFitDataTab::makeRawWorkspace(const std::string& wsName)
{
  Mantid::API::Workspace_sptr inputWs = boost::dynamic_pointer_cast<Mantid::API::Workspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsName) );
  Mantid::API::IAlgorithm_sptr duplicate = Mantid::API::AlgorithmManager::Instance().create("CloneWorkspace");
  duplicate->initialize();
  duplicate->setProperty<Mantid::API::Workspace_sptr>("InputWorkspace", inputWs);
  duplicate->setPropertyValue("OutputWorkspace", wsName + "_Raw");
  duplicate->execute();

  Mantid::API::Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
  Mantid::API::MatrixWorkspace_sptr outputWs = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(temp);
  //Mantid::API::AnalysisDataService::Instance().add(wsName + "_Raw", outputWs);
}


/**
* Groups the given workspace group with the raw workspace that is associated with
* the workspace name which is also given.
*
* @params wsName :: The name of the workspace the raw file is associated to.
* @params wsGroupName :: The name of the workspaceGroup to join with and what to call the output.
*/
void MuonAnalysisFitDataTab::groupRawWorkspace(const std::string& wsName, const std::string & wsGroupName)
{
  std::vector<std::string> inputWorkspaces;
  inputWorkspaces.push_back(wsName);
  inputWorkspaces.push_back(wsGroupName);

  Mantid::API::IAlgorithm_sptr groupingAlg = Mantid::API::AlgorithmManager::Instance().create("GroupWorkspaces");
  groupingAlg->initialize();
  groupingAlg->setProperty("InputWorkspaces", inputWorkspaces);
  groupingAlg->setPropertyValue("OutputWorkspace", wsGroupName);
  groupingAlg->execute();
}


}
}
}