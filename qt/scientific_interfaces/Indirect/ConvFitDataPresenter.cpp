// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFitDataPresenter.h"
#include "ConvFitAddWorkspaceDialog.h"
#include "ConvFitDataTablePresenter.h"

#include "MantidAPI/AnalysisDataService.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

ConvFitDataPresenter::ConvFitDataPresenter(ConvFitModel *model, IIndirectFitDataView *view)
    : IndirectFitDataPresenter(model, view, std::make_unique<ConvFitDataTablePresenter>(model, view->getDataTable())),
      m_convModel(model) {
  connect(view, SIGNAL(resolutionLoaded(const QString &)), this, SLOT(setModelResolution(const QString &)));
  connect(view, SIGNAL(resolutionLoaded(const QString &)), this, SIGNAL(singleResolutionLoaded()));
}

void ConvFitDataPresenter::setModelResolution(const QString &name) {
  auto const workspaceCount = m_convModel->getNumberOfWorkspaces();
  auto const index = m_convModel->getWorkspace(WorkspaceID{0}) ? workspaceCount - WorkspaceID{1} : workspaceCount;
  setModelResolution(name.toStdString(), index);
}

void ConvFitDataPresenter::setModelResolution(std::string const &name, WorkspaceID const &workspaceIndex) {
  try {
    m_convModel->setResolution(name, workspaceIndex);
    emit modelResolutionAdded(name, workspaceIndex);
  } catch (std::exception const &ex) {
    displayWarning(ex.what());
  }
}

void ConvFitDataPresenter::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto convDialog = dynamic_cast<ConvFitAddWorkspaceDialog const *>(dialog)) {
    addWorkspace(*convDialog, *m_convModel);
    auto const name = convDialog->resolutionName();
    auto const index = m_convModel->getNumberOfWorkspaces() - WorkspaceID{1};
    m_convModel->setResolution(name, index);
    emit modelResolutionAdded(name, index);
  }
}

void ConvFitDataPresenter::addWorkspace(ConvFitAddWorkspaceDialog const &dialog, IndirectFittingModel &model) {
  model.addWorkspace(dialog.workspaceName(), dialog.workspaceIndices());
}

std::unique_ptr<IAddWorkspaceDialog> ConvFitDataPresenter::getAddWorkspaceDialog(QWidget *parent) const {
  auto dialog = std::make_unique<ConvFitAddWorkspaceDialog>(parent);
  dialog->setResolutionWSSuffices(getResolutionWSSuffices());
  dialog->setResolutionFBSuffices(getResolutionFBSuffices());
  return dialog;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
