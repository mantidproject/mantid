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
#include "MantidKernel/Logger.h"

#include <boost/utility/in_place_factory.hpp>

#include <string>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Preview Model");
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

void PreviewModel::loadWorkspace(std::string const &workspaceName, IJobManager &jobManager) {
  createRunDetails(workspaceName);
  auto ws = loadFromAds(workspaceName);
  if (!ws) {
    // Row is automatically updated (as we pass by-ref) on completion
    jobManager.startPreprocessing(*m_runDetails);
  } else {
    m_runDetails->setLoadedWs(ws);
  }
}

MatrixWorkspace_sptr PreviewModel::getLoadedWs() const { return m_runDetails->getLoadedWs(); }

MatrixWorkspace_sptr PreviewModel::loadFromAds(std::string const &workspaceName) const {
  auto &adsInstance = AnalysisDataService::Instance();
  if (adsInstance.doesExist(workspaceName)) {
    g_log.information("Loaded from ADS" + workspaceName);
    return adsInstance.retrieveWS<MatrixWorkspace>(workspaceName);
  }
  return nullptr;
}

void PreviewModel::createRunDetails(const std::string &workspaceName) {
  m_runDetails = std::make_unique<PreviewRow>(std::vector<std::string>{workspaceName});
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
