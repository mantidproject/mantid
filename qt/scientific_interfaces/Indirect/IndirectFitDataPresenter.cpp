// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitDataPresenter.h"

#include <utility>

#include "IndirectAddWorkspaceDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitDataPresenter::IndirectFitDataPresenter(IIndirectFittingModel *model, IIndirectFitDataView *view)
    : IndirectFitDataPresenter(
          model, view,
          std::make_unique<IndirectFitDataTablePresenter>(model->getFitDataModel(), view->getDataTable())) {}

IndirectFitDataPresenter::IndirectFitDataPresenter(IIndirectFittingModel *model, IIndirectFitDataView *view,
                                                   std::unique_ptr<IndirectFitDataTablePresenter> tablePresenter)
    : m_model(model), m_view(view), m_tablePresenter(std::move(tablePresenter)) {
  observeReplace(true);

  connect(m_view, SIGNAL(addClicked()), this, SIGNAL(requestedAddWorkspaceDialog()));
  connect(m_view, SIGNAL(addClicked()), this, SLOT(showAddWorkspaceDialog()));

  connect(m_view, SIGNAL(removeClicked()), m_tablePresenter.get(), SLOT(removeSelectedData()));
  connect(m_view, SIGNAL(removeClicked()), this, SIGNAL(dataRemoved()));
  connect(m_view, SIGNAL(removeClicked()), this, SIGNAL(dataChanged()));
  connect(m_view, SIGNAL(startXChanged(double)), this, SIGNAL(startXChanged(double)));
  connect(m_view, SIGNAL(endXChanged(double)), this, SIGNAL(endXChanged(double)));

  connect(m_tablePresenter.get(), SIGNAL(startXChanged(double, TableDatasetIndex, WorkspaceIndex)), this,
          SIGNAL(startXChanged(double, TableDatasetIndex, WorkspaceIndex)));
  connect(m_tablePresenter.get(), SIGNAL(endXChanged(double, TableDatasetIndex, WorkspaceIndex)), this,
          SIGNAL(endXChanged(double, TableDatasetIndex, WorkspaceIndex)));
  connect(m_tablePresenter.get(), SIGNAL(excludeRegionChanged(const std::string &, TableDatasetIndex, WorkspaceIndex)),
          this, SIGNAL(excludeRegionChanged(const std::string &, TableDatasetIndex, WorkspaceIndex)));
}

IndirectFitDataPresenter::~IndirectFitDataPresenter() { observeReplace(false); }

IIndirectFitDataView const *IndirectFitDataPresenter::getView() const { return m_view; }

void IndirectFitDataPresenter::setSampleWSSuffices(const QStringList &suffixes) { m_wsSampleSuffixes = suffixes; }

void IndirectFitDataPresenter::setSampleFBSuffices(const QStringList &suffixes) { m_fbSampleSuffixes = suffixes; }

void IndirectFitDataPresenter::setResolutionWSSuffices(const QStringList &suffixes) {
  m_wsResolutionSuffixes = suffixes;
}

void IndirectFitDataPresenter::setResolutionFBSuffices(const QStringList &suffixes) {
  m_fbResolutionSuffixes = suffixes;
}

void IndirectFitDataPresenter::setStartX(double startX, TableDatasetIndex, WorkspaceIndex) {
  m_tablePresenter->updateTableFromModel();
}

void IndirectFitDataPresenter::setStartX(double startX, TableDatasetIndex) { m_tablePresenter->updateTableFromModel(); }

void IndirectFitDataPresenter::setEndX(double endX, TableDatasetIndex, WorkspaceIndex) {
  m_tablePresenter->updateTableFromModel();
}

void IndirectFitDataPresenter::setEndX(double endX, TableDatasetIndex) { m_tablePresenter->updateTableFromModel(); }

void IndirectFitDataPresenter::setExclude(const std::string &, TableDatasetIndex, WorkspaceIndex) {
  m_tablePresenter->updateTableFromModel();
}

void IndirectFitDataPresenter::updateSpectraInTable(TableDatasetIndex) { m_tablePresenter->updateTableFromModel(); }

void IndirectFitDataPresenter::updateDataInTable(TableDatasetIndex) { m_tablePresenter->updateTableFromModel(); }

DataForParameterEstimationCollection
IndirectFitDataPresenter::getDataForParameterEstimation(const EstimationDataSelector &selector) const {
  return m_model->getDataForParameterEstimation(std::move(selector));
}

UserInputValidator &IndirectFitDataPresenter::validate(UserInputValidator &validator) {
  return m_view->validate(validator);
}

void IndirectFitDataPresenter::showAddWorkspaceDialog() {
  if (!m_addWorkspaceDialog)
    m_addWorkspaceDialog = getAddWorkspaceDialog(m_view->parentWidget());
  m_addWorkspaceDialog->setWSSuffices(m_wsSampleSuffixes);
  m_addWorkspaceDialog->setFBSuffices(m_fbSampleSuffixes);
  m_addWorkspaceDialog->updateSelectedSpectra();
  m_addWorkspaceDialog->show();
  connect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this, SLOT(addData()));
  connect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this, SLOT(closeDialog()));
}

std::unique_ptr<IAddWorkspaceDialog> IndirectFitDataPresenter::getAddWorkspaceDialog(QWidget *parent) const {
  return std::make_unique<IndirectAddWorkspaceDialog>(parent);
}

void IndirectFitDataPresenter::addData() { addData(m_addWorkspaceDialog.get()); }

void IndirectFitDataPresenter::closeDialog() {
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this, SLOT(addData()));
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this, SLOT(closeDialog()));
  m_addWorkspaceDialog->close();
  m_addWorkspaceDialog = nullptr;
}

void IndirectFitDataPresenter::addData(IAddWorkspaceDialog const *dialog) {
  try {
    addDataToModel(dialog);
    m_tablePresenter->updateTableFromModel();
    emit dataAdded();
    emit dataChanged();
  } catch (const std::runtime_error &ex) {
    displayWarning(ex.what());
  }
}

void IndirectFitDataPresenter::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto indirectDialog = dynamic_cast<IndirectAddWorkspaceDialog const *>(dialog))
    m_model->addWorkspace(indirectDialog->workspaceName(), indirectDialog->workspaceIndices());
}

void IndirectFitDataPresenter::displayWarning(const std::string &warning) { m_view->displayWarning(warning); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
