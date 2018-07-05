#include "ConvFitDataPresenter.h"
#include "ConvFitAddWorkspaceDialog.h"
#include "ConvFitDataTablePresenter.h"

#include "MantidKernel/make_unique.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

ConvFitDataPresenter::ConvFitDataPresenter(ConvFitModel *model,
                                           IndirectFitDataView *view)
    : IndirectFitDataPresenter(
          model, view,
          new ConvFitDataTablePresenter(model, view->getDataTable())),
      m_convModel(model) {
  setResolutionHidden(false);

  connect(view, SIGNAL(resolutionLoaded(const QString &)), this,
          SLOT(setModelResolution(const QString &)));
  connect(view, SIGNAL(resolutionLoaded(const QString &)), this,
          SIGNAL(singleResolutionLoaded()));
}

void ConvFitDataPresenter::setModelResolution(const QString &name) {
  m_convModel->setResolution(name.toStdString(),
                             m_convModel->numberOfWorkspaces() - 1);
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
  if (!resolution.empty())
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
