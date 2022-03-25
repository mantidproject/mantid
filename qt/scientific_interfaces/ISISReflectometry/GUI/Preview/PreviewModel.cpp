// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewModel.h"
#include "GUI/Common/IJobManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"

#include <boost/utility/in_place_factory.hpp>

#include <string>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Preview Model");
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

PreviewModel::PreviewModel() {
  // This simplifies testing greatly, as it ensures that m_runDetails is never null
  createRunDetails("");
}

/** Set the loaded workspace from the ADS if it exists
 *
 * @param workspaceName : the workspace name to look for
 * @returns : true if the loaded workspace was set, false if it was not found in the ADS
 * @throws : if the workspace exists in the ADS but is an unexpected type
 */
bool PreviewModel::loadWorkspaceFromAds(std::string const &workspaceName) {
  auto &adsInstance = AnalysisDataService::Instance();
  if (!adsInstance.doesExist(workspaceName)) {
    return false;
  }
  auto ws = adsInstance.retrieveWS<MatrixWorkspace>(workspaceName);
  if (!ws) {
    throw std::runtime_error("Unsupported workspace type; expected MatrixWorkspace");
  }

  createRunDetails(workspaceName);
  m_runDetails->setLoadedWs(ws);
  return true;
}

/** Load a workspace and perform standard ISIS reflectometry preprocessing on it.
 * This is done asynchronously. The caller should subscribe to the job manager to
 * get a callback when loading is finished.
 *
 * @param workspaceName : the workspace name to load
 * @param jobManager : the job manager that will perform the loading
 */
void PreviewModel::loadAndPreprocessWorkspaceAsync(std::string const &workspaceName, IJobManager &jobManager) {
  createRunDetails(workspaceName);
  // Note that the run details are automatically updated with the resulting workspace (as we pass by-ref) on completion
  jobManager.startPreprocessing(*m_runDetails);
}

/** Sum spectra across banks
 *
 * @param wsIndices : the workspace indices of the spectra to sum
 * @param jobManager : the job manager that will execute the algorithm
 */
void PreviewModel::sumBanksAsync(IJobManager &jobManager) { jobManager.startSumBanks(*m_runDetails); }

MatrixWorkspace_sptr PreviewModel::getLoadedWs() const { return m_runDetails->getLoadedWs(); }
MatrixWorkspace_sptr PreviewModel::getSummedWs() const { return m_runDetails->getSummedWs(); }

std::vector<Mantid::detid_t> PreviewModel::getSelectedBanks() const { return m_runDetails->getSelectedBanks(); }

void PreviewModel::setSelectedBanks(std::vector<Mantid::detid_t> selectedBanks) {
  m_runDetails->setSelectedBanks(std::move(selectedBanks));
}

void PreviewModel::createRunDetails(const std::string &workspaceName) {
  m_runDetails = std::make_unique<PreviewRow>(std::vector<std::string>{workspaceName});
}

std::string PreviewModel::detIDsToString(std::vector<Mantid::detid_t> const &indices) const {
  return Mantid::Kernel::Strings::simpleJoin(indices.cbegin(), indices.cend(), ",");
}

void PreviewModel::exportSummedWsToAds() const {
  if (auto summedWs = m_runDetails->getSummedWs()) {
    AnalysisDataService::Instance().addOrReplace("preview_summed_ws", summedWs);
  } else {
    g_log.error("Could not export summed WS. No rectangular selection has been made on the instrument viewer.");
  }
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
