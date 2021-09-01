// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewModel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"

#include <memory>
#include <string>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Preview Model");
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

void PreviewModel::loadWorkspace(std::string const &workspaceName) {
  auto &adsInstance = AnalysisDataService::Instance();
  if (adsInstance.doesExist(workspaceName)) {
    m_instViewWorkspace = adsInstance.retrieveWS<MatrixWorkspace>(workspaceName);
    g_log.information("Loaded " + workspaceName);
    return;
  }
  // TODO load
}

MatrixWorkspace_sptr PreviewModel::getInstViewWorkspace() const { return m_instViewWorkspace; }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
