// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqFitModel.h"

#include <utility>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Logger.h"

using namespace Mantid::API;

namespace {

Mantid::Kernel::Logger logger("FqFitModel");

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

FqFitModel::FqFitModel() : m_adsInstance(Mantid::API::AnalysisDataService::Instance()) { m_fitType = FQFIT_STRING; }

void FqFitModel::removeWorkspace(WorkspaceID workspaceID) { IndirectFittingModel::removeWorkspace(workspaceID); }

std::string FqFitModel::getFitParameterName(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  const auto ws = getWorkspace(workspaceID);
  const auto axis = dynamic_cast<TextAxis *>(ws->getAxis(1));
  return axis->label(spectrum.value);
}

bool FqFitModel::isMultiFit() const {
  if (getNumberOfWorkspaces() == WorkspaceID{0})
    return false;
  return !allWorkspacesEqual(getWorkspace(WorkspaceID{0}));
}

std::string FqFitModel::getResultXAxisUnit() const { return ""; }

std::string FqFitModel::getResultLogName() const { return "SourceName"; }

bool FqFitModel::allWorkspacesEqual(const Mantid::API::MatrixWorkspace_sptr &workspace) const {
  for (auto i = WorkspaceID{1}; i < getNumberOfWorkspaces(); ++i) {
    if (getWorkspace(i) != workspace)
      return false;
  }
  return true;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
