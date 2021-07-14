// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqFitDataPresenter.h"
#include "IDAFunctionParameterEstimation.h"
#include "MantidAPI/TextAxis.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

FqFitDataPresenter::FqFitDataPresenter(FqFitModel *model, IIndirectFitDataView *view,
                                       IFQFitObserver *SingleFunctionTemplateBrowser)
    : IndirectFitDataPresenter(model->getFitDataModel(), view), m_activeParameterType("Width"),
      m_activeWorkspaceID(WorkspaceID{0}), m_fqFitModel(model),
      m_adsInstance(Mantid::API::AnalysisDataService::Instance()) {
  connect(this, SIGNAL(requestedAddWorkspaceDialog()), this, SLOT(updateActiveWorkspaceID()));

  m_notifier = Notifier<IFQFitObserver>();
  m_notifier.subscribe(SingleFunctionTemplateBrowser);
}

void FqFitDataPresenter::setActiveParameterType(const std::string &type) { m_activeParameterType = type; }

void FqFitDataPresenter::updateActiveWorkspaceID() { m_activeWorkspaceID = m_fqFitModel->getNumberOfWorkspaces(); }

void FqFitDataPresenter::updateActiveWorkspaceID(WorkspaceID index) { m_activeWorkspaceID = index; }

void FqFitDataPresenter::setDialogParameterNames(FqFitAddWorkspaceDialog *dialog, const std::string &workspaceName) {
  FqFitParameters parameters;
  try {
    auto workspace = m_adsInstance.retrieveWS<MatrixWorkspace>(workspaceName);
    parameters = m_fqFitModel->createFqFitParameters(workspace.get());
    dialog->enableParameterSelection();
  } catch (const std::invalid_argument &) {
    dialog->disableParameterSelection();
  }
  updateParameterTypes(dialog, parameters);
  updateParameterOptions(dialog, parameters);
}

void FqFitDataPresenter::dialogParameterTypeUpdated(FqFitAddWorkspaceDialog *dialog, const std::string &type) {
  const auto workspaceName = dialog->workspaceName();
  auto workspace = m_adsInstance.retrieveWS<MatrixWorkspace>(workspaceName);
  const auto parameter = m_fqFitModel->createFqFitParameters(workspace.get());
  setActiveParameterType(type);
  updateParameterOptions(dialog, parameter);
}

void FqFitDataPresenter::updateParameterOptions(FqFitAddWorkspaceDialog *dialog, FqFitParameters parameter) {
  setActiveWorkspaceIDToCurrentWorkspace(dialog);
  setActiveParameterType(dialog->parameterType());
  if (m_activeParameterType == "Width")
    dialog->setParameterNames(parameter.widths);
  else if (m_activeParameterType == "EISF")
    dialog->setParameterNames(parameter.eisf);
  else
    dialog->setParameterNames({});
}

void FqFitDataPresenter::updateParameterTypes(FqFitAddWorkspaceDialog *dialog, FqFitParameters &parameters) {
  setActiveWorkspaceIDToCurrentWorkspace(dialog);
  dialog->setParameterTypes(getParameterTypes(parameters));
}

std::vector<std::string> FqFitDataPresenter::getParameterTypes(FqFitParameters &parameters) const {
  std::vector<std::string> types;
  if (!parameters.widths.empty())
    types.emplace_back("Width");
  if (!parameters.eisf.empty())
    types.emplace_back("EISF");
  return types;
}

void FqFitDataPresenter::setActiveWorkspaceIDToCurrentWorkspace(IAddWorkspaceDialog const *dialog) {
  //  update active data index with correct index based on the workspace name
  //  and the vector in m_fitDataModel which is in the base class
  //  indirectFittingModel get table workspace index
  const auto wsName = dialog->workspaceName().append("_HWHM");
  // This a vector of workspace names currently loaded
  auto wsVector = m_fqFitModel->getFitDataModel()->getWorkspaceNames();
  // this is an iterator pointing to the current wsName in wsVector
  auto wsIt = std::find(wsVector.begin(), wsVector.end(), wsName);
  // this is the index of the workspace.
  const auto index = WorkspaceID(std::distance(wsVector.begin(), wsIt));
  updateActiveWorkspaceID(index);
}

std::unique_ptr<IAddWorkspaceDialog> FqFitDataPresenter::getAddWorkspaceDialog(QWidget *parent) const {
  auto dialog = std::make_unique<FqFitAddWorkspaceDialog>(parent);
  connect(dialog.get(), SIGNAL(workspaceChanged(FqFitAddWorkspaceDialog *, const std::string &)), this,
          SLOT(setDialogParameterNames(FqFitAddWorkspaceDialog *, const std::string &)));
  connect(dialog.get(), SIGNAL(parameterTypeChanged(FqFitAddWorkspaceDialog *, const std::string &)), this,
          SLOT(dialogParameterTypeUpdated(FqFitAddWorkspaceDialog *, const std::string &)));
  return dialog;
}

void FqFitDataPresenter::addTableEntry(FitDomainIndex row) {
  const auto &name = m_model->getWorkspace(row)->getName();

  auto subIndices = m_model->getSubIndices(row);
  const auto workspace = m_model->getWorkspace(subIndices.first);
  const auto axis = dynamic_cast<Mantid::API::TextAxis *>(workspace->getAxis(1));
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
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
