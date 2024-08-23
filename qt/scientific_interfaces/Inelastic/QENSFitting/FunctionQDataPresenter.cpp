// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionQDataPresenter.h"

#include "FitTab.h"
#include "FitTabConstants.h"
#include "ParameterEstimation.h"

#include "MantidAPI/TextAxis.h"

namespace {
using namespace MantidQt::CustomInterfaces::Inelastic;

std::string createSpectra(const std::vector<std::size_t> &spectrum) {
  std::string spectra = "";
  for (const auto spec : spectrum) {
    spectra.append(std::to_string(spec) + ",");
  }
  return spectra;
}

void replaceAxisLabel(TextAxis *axis, std::size_t const index, std::string const &fromStr, std::string const &toStr) {
  auto label = axis->label(index);
  auto const position = label.find(fromStr);
  if (position == std::string::npos) {
    return;
  }
  label.replace(position, fromStr.length(), toStr);
  axis->setLabel(index, label);
}

void convertWidthToHWHM(MatrixWorkspace_sptr workspace, const std::vector<std::size_t> &widthSpectra) {
  for (auto const &spectrumIndex : widthSpectra) {
    if (auto axis = dynamic_cast<TextAxis *>(workspace->getAxis(1))) {
      replaceAxisLabel(axis, spectrumIndex, "Width", "HWHM");
      replaceAxisLabel(axis, spectrumIndex, "FWHM", "HWHM");
      workspace->mutableY(spectrumIndex) *= 0.5;
      workspace->mutableE(spectrumIndex) *= 0.5;
    }
  }
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

FunctionQDataPresenter::FunctionQDataPresenter(IFitTab *tab, IDataModel *model, IFitDataView *view)
    : FitDataPresenter(tab, model, view), m_activeParameterType("Width"), m_activeWorkspaceID(WorkspaceID{0}),
      m_adsInstance(Mantid::API::AnalysisDataService::Instance()) {}

bool FunctionQDataPresenter::addWorkspaceFromDialog(MantidWidgets::IAddWorkspaceDialog const *dialog) {
  if (const auto functionQDialog = dynamic_cast<FunctionQAddWorkspaceDialog const *>(dialog)) {
    addWorkspace(functionQDialog->workspaceName(), functionQDialog->parameterType(),
                 functionQDialog->parameterNameIndex());
    setActiveWorkspaceIDToCurrentWorkspace(functionQDialog);

    auto const parameterIndex = functionQDialog->parameterNameIndex();
    if (parameterIndex < 0) {
      throw std::runtime_error("No valid parameter was selected.");
    }
    FunctionQParameters parameters(m_model->getWorkspace(m_activeWorkspaceID));
    setActiveSpectra(parameters.spectra(functionQDialog->parameterType()), static_cast<std::size_t>(parameterIndex),
                     m_activeWorkspaceID, false);
    updateActiveWorkspaceID(getNumberOfWorkspaces());
    return true;
  }
  return false;
}

void FunctionQDataPresenter::addWorkspace(const std::string &workspaceName, const std::string &paramType,
                                          const int &spectrum_index) {
  const auto workspace = m_adsInstance.retrieveWS<MatrixWorkspace>(workspaceName);
  FunctionQParameters parameters(workspace);
  if (!parameters)
    throw std::invalid_argument("Workspace contains no Width, EISF or A0 spectra.");

  if (workspace->y(0).size() == 1)
    throw std::invalid_argument("Workspace contains only one data point.");

  convertWidthToHWHM(workspace, parameters.spectra("Width"));
  m_tab->handleFunctionListChanged(chooseFunctionQFunctions(paramType == "Width"));
  const auto singleSpectra = FunctionModelSpectra(std::to_string(parameters.spectra(paramType)[spectrum_index]));
  m_model->addWorkspace(workspace->getName(), singleSpectra);
}

std::map<std::string, std::string> FunctionQDataPresenter::chooseFunctionQFunctions(bool paramWidth) const {
  if (m_view->isTableEmpty()) // when first data is added to table, it can only be either WIDTH or EISF
    return paramWidth ? FunctionQ::WIDTH_FITS : FunctionQ::EISF_FITS;

  bool widthFuncs = paramWidth || m_view->dataColumnContainsText("HWHM");
  bool eisfFuncs = !paramWidth || m_view->dataColumnContainsText("EISF") || m_view->dataColumnContainsText("A0");
  if (widthFuncs && eisfFuncs)
    return FunctionQ::ALL_FITS;
  else if (widthFuncs)
    return FunctionQ::WIDTH_FITS;
  else
    return FunctionQ::EISF_FITS;
}

void FunctionQDataPresenter::setActiveParameterType(const std::string &type) { m_activeParameterType = type; }

void FunctionQDataPresenter::updateActiveWorkspaceID(WorkspaceID index) { m_activeWorkspaceID = index; }

void FunctionQDataPresenter::handleAddClicked() { updateActiveWorkspaceID(m_model->getNumberOfWorkspaces()); }

void FunctionQDataPresenter::handleWorkspaceChanged(FunctionQAddWorkspaceDialog *dialog,
                                                    const std::string &workspaceName) {
  try {
    auto workspace = m_adsInstance.retrieveWS<MatrixWorkspace>(workspaceName);
    FunctionQParameters parameters(workspace);
    dialog->enableParameterSelection();
    updateParameterTypes(dialog, parameters);
    updateParameterOptions(dialog, parameters);
  } catch (const std::invalid_argument &) {
    FunctionQParameters parameters;
    dialog->disableParameterSelection();
    updateParameterTypes(dialog, parameters);
    updateParameterOptions(dialog, parameters);
  }
}

void FunctionQDataPresenter::handleParameterTypeChanged(FunctionQAddWorkspaceDialog *dialog, const std::string &type) {
  const auto workspaceName = dialog->workspaceName();
  if (!workspaceName.empty() && m_adsInstance.doesExist(workspaceName)) {
    auto workspace = m_adsInstance.retrieveWS<MatrixWorkspace>(workspaceName);
    FunctionQParameters parameters(workspace);
    setActiveParameterType(type);
    updateParameterOptions(dialog, parameters);
  }
}

void FunctionQDataPresenter::updateParameterOptions(FunctionQAddWorkspaceDialog *dialog,
                                                    const FunctionQParameters &parameter) {
  setActiveWorkspaceIDToCurrentWorkspace(dialog);
  setActiveParameterType(dialog->parameterType());
  dialog->setParameterNames(parameter.names(m_activeParameterType));
}

void FunctionQDataPresenter::updateParameterTypes(FunctionQAddWorkspaceDialog *dialog,
                                                  FunctionQParameters const &parameters) {
  setActiveWorkspaceIDToCurrentWorkspace(dialog);
  dialog->setParameterTypes(parameters.types());
}

void FunctionQDataPresenter::setActiveWorkspaceIDToCurrentWorkspace(MantidWidgets::IAddWorkspaceDialog const *dialog) {
  //  update active data index with correct index based on the workspace name
  //  and the vector in m_fitDataModel which is in the base class
  //  FittingModel get table workspace index
  auto const &wsName = dialog->workspaceName();
  // This a vector of workspace names currently loaded
  auto const wsVector = m_model->getWorkspaceNames();
  // this is an iterator pointing to the current wsName in wsVector
  auto wsIt = std::find(wsVector.cbegin(), wsVector.cend(), wsName);
  if (wsIt == wsVector.cend()) {
    return;
  }
  // this is the index of the workspace.
  auto const index = WorkspaceID(std::distance(wsVector.cbegin(), wsIt));
  updateActiveWorkspaceID(index);
}

void FunctionQDataPresenter::setActiveSpectra(std::vector<std::size_t> const &activeParameterSpectra,
                                              std::size_t parameterIndex, WorkspaceID workspaceID, bool single) {
  if (activeParameterSpectra.size() <= parameterIndex) {
    return;
  }
  if (single) {
    m_model->setSpectra(createSpectra(std::vector<std::size_t>({activeParameterSpectra[parameterIndex]})), workspaceID);
    return;
  }
  // In multiple mode the spectra needs to be appending on the existing spectra list.
  auto spectra = std::vector<std::size_t>({activeParameterSpectra[parameterIndex]});
  for (const auto &i : m_model->getSpectra(workspaceID)) {
    if ((std::find(spectra.begin(), spectra.end(), i.value) == spectra.end())) {
      spectra.emplace_back(i.value);
    }
  }
  m_model->setSpectra(createSpectra(spectra), workspaceID);
}

void FunctionQDataPresenter::addTableEntry(FitDomainIndex row) {
  const auto &name = m_model->getWorkspace(row)->getName();

  const auto subIndices = m_model->getSubIndices(row);
  const auto workspace = m_model->getWorkspace(subIndices.first);
  const auto axis = dynamic_cast<Mantid::API::TextAxis const *>(workspace->getAxis(1));
  if (!axis) {
    return;
  }
  const auto parameter = axis->label(subIndices.second.value);

  const auto workspaceIndex = m_model->getSpectrum(row);
  const auto range = m_model->getFittingRange(row);
  const auto exclude = m_model->getExcludeRegion(row);

  FitDataRow newRow;
  newRow.name = name;
  newRow.workspaceIndex = workspaceIndex;
  newRow.parameter = parameter;
  newRow.startX = range.first;
  newRow.endX = range.second;
  newRow.exclude = exclude;
  m_view->addTableEntry(row.value, newRow);
}
} // namespace MantidQt::CustomInterfaces::Inelastic
