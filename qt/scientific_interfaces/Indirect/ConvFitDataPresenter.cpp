// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFitDataPresenter.h"
#include "ConvFitAddWorkspaceDialog.h"
#include "ConvFitDataTablePresenter.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/make_unique.h"

namespace {
using namespace Mantid::API;

bool isWorkspaceLoaded(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

ConvFitDataPresenter::ConvFitDataPresenter(ConvFitModel *model,
                                           IIndirectFitDataView *view)
    : IndirectFitDataPresenter(
          model, view,
          Mantid::Kernel::make_unique<ConvFitDataTablePresenter>(
              model, view->getDataTable())),
      m_convModel(model) {
  setResolutionHidden(false);

  connect(view, SIGNAL(resolutionLoaded(const QString &)), this,
          SLOT(setModelResolution(const QString &)));
  connect(view, SIGNAL(resolutionLoaded(const QString &)), this,
          SIGNAL(singleResolutionLoaded()));
}

void ConvFitDataPresenter::setModelResolution(const QString &name) {
  auto const workspaceCount = m_convModel->numberOfWorkspaces();
  auto const index =
      m_convModel->getWorkspace(0) ? workspaceCount - 1 : workspaceCount;
  setModelResolution(name.toStdString(), index);
}

void ConvFitDataPresenter::setModelResolution(std::string const &name,
                                              std::size_t const &index) {
  try {
    m_convModel->setResolution(name, index);
  } catch (std::exception const &ex) {
    displayWarning(ex.what());
  }
}

void ConvFitDataPresenter::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto convDialog =
          dynamic_cast<ConvFitAddWorkspaceDialog const *>(dialog)) {
    addWorkspace(convDialog, m_convModel);
    m_convModel->setResolution(convDialog->resolutionName(),
                               m_convModel->numberOfWorkspaces() - 1);
  }
}

void ConvFitDataPresenter::addWorkspace(ConvFitAddWorkspaceDialog const *dialog,
                                        IndirectFittingModel *model) {
  model->addWorkspace(dialog->workspaceName(), dialog->workspaceIndices());
}

void ConvFitDataPresenter::addModelData(const std::string &name) {
  IndirectFitDataPresenter::addModelData(name);
  const auto resolution = getView()->getSelectedResolution();
  if (!resolution.empty() && isWorkspaceLoaded(resolution))
    m_convModel->setResolution(resolution, 0);
}

std::unique_ptr<IAddWorkspaceDialog>
ConvFitDataPresenter::getAddWorkspaceDialog(QWidget *parent) const {
  auto dialog = Mantid::Kernel::make_unique<ConvFitAddWorkspaceDialog>(parent);
  dialog->setResolutionFBSuffices(getView()->getResolutionFBSuffices());
  dialog->setResolutionWSSuffices(getView()->getResolutionWSSuffices());
  return std::move(dialog);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
