// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFitDataPresenter.h"
#include "ConvFitAddWorkspaceDialog.h"

#include "MantidAPI/AnalysisDataService.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

ConvFitDataPresenter::ConvFitDataPresenter(IIndirectFitDataModel *model, IIndirectFitDataView *view)
    : IndirectFitDataPresenter(model, view) {
  connect(view, SIGNAL(resolutionLoaded(const QString &)), this, SLOT(setModelResolution(const QString &)));
  connect(view, SIGNAL(resolutionLoaded(const QString &)), this, SIGNAL(singleResolutionLoaded()));
}

void ConvFitDataPresenter::setModelResolution(const QString &name) {
  auto const workspaceCount = m_model->getNumberOfWorkspaces();
  auto const index = m_model->getWorkspace(WorkspaceID{0}) ? workspaceCount - WorkspaceID{1} : workspaceCount;
  setModelResolution(name.toStdString(), index);
}

void ConvFitDataPresenter::setModelResolution(std::string const &name, WorkspaceID const &workspaceID) {
  try {
    m_model->setResolution(name, workspaceID);
    emit modelResolutionAdded(name, workspaceID);
  } catch (std::exception const &ex) {
    displayWarning(ex.what());
  }
}

std::unique_ptr<IAddWorkspaceDialog> ConvFitDataPresenter::getAddWorkspaceDialog(QWidget *parent) const {
  auto dialog = std::make_unique<ConvFitAddWorkspaceDialog>(parent);
  dialog->setResolutionWSSuffices(getResolutionWSSuffices());
  dialog->setResolutionFBSuffices(getResolutionFBSuffices());
  return dialog;
}

void ConvFitDataPresenter::addTableEntry(FitDomainIndex row) {
  const auto &name = m_model->getWorkspace(row)->getName();
  auto resolutionVector = m_model->getResolutionsForFit();
  const auto resolution = resolutionVector.at(row.value).first;
  const auto workspaceIndex = m_model->getSpectrum(row);
  const auto range = m_model->getFittingRange(row);
  const auto exclude = m_model->getExcludeRegion(row);

  FitDataRow newRow;
  newRow.name = name;
  newRow.workspaceIndex = workspaceIndex;
  newRow.resolution = resolution;
  newRow.startX = range.first;
  newRow.endX = range.second;
  newRow.exclude = exclude;

  m_view->addTableEntry(row.value, newRow);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
