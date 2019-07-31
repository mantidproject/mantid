// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitDataPresenter.h"
#include "IndirectAddWorkspaceDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitDataPresenter::IndirectFitDataPresenter(IndirectFittingModel *model,
                                                   IIndirectFitDataView *view)
    : IndirectFitDataPresenter(model, view,
                               std::make_unique<IndirectDataTablePresenter>(
                                   model, view->getDataTable())) {}

IndirectFitDataPresenter::IndirectFitDataPresenter(
    IndirectFittingModel *model, IIndirectFitDataView *view,
    std::unique_ptr<IndirectDataTablePresenter> tablePresenter)
    : m_model(model), m_view(view),
      m_tablePresenter(std::move(tablePresenter)) {
  observeReplace(true);

  connect(m_view, SIGNAL(singleDataViewSelected()), this,
          SLOT(setModelFromSingleData()));
  connect(m_view, SIGNAL(multipleDataViewSelected()), this,
          SLOT(setModelFromMultipleData()));

  connect(m_view, SIGNAL(singleDataViewSelected()), this,
          SIGNAL(singleDataViewSelected()));
  connect(m_view, SIGNAL(multipleDataViewSelected()), this,
          SIGNAL(multipleDataViewSelected()));

  connect(m_view, SIGNAL(sampleLoaded(const QString &)), this,
          SLOT(setModelWorkspace(const QString &)));
  connect(m_view, SIGNAL(sampleLoaded(const QString &)), this,
          SIGNAL(dataChanged()));

  connect(m_view, SIGNAL(addClicked()), this,
          SIGNAL(requestedAddWorkspaceDialog()));
  connect(m_view, SIGNAL(addClicked()), this, SLOT(showAddWorkspaceDialog()));

  connect(m_view, SIGNAL(removeClicked()), m_tablePresenter.get(),
          SLOT(removeSelectedData()));
  connect(m_view, SIGNAL(removeClicked()), this, SIGNAL(dataRemoved()));
  connect(m_view, SIGNAL(removeClicked()), this, SIGNAL(dataChanged()));
  connect(m_view, SIGNAL(startXChanged(double)), this,
          SIGNAL(startXChanged(double)));
  connect(m_view, SIGNAL(endXChanged(double)), this,
          SIGNAL(endXChanged(double)));

  connect(m_tablePresenter.get(),
          SIGNAL(startXChanged(double, DatasetIndex, WorkspaceIndex)), this,
          SIGNAL(startXChanged(double, DatasetIndex, WorkspaceIndex)));
  connect(m_tablePresenter.get(),
          SIGNAL(endXChanged(double, DatasetIndex, WorkspaceIndex)), this,
          SIGNAL(endXChanged(double, DatasetIndex, WorkspaceIndex)));
  connect(m_tablePresenter.get(),
          SIGNAL(excludeRegionChanged(const std::string &, DatasetIndex,
                                      WorkspaceIndex)),
          this,
          SIGNAL(excludeRegionChanged(const std::string &, DatasetIndex,
                                      WorkspaceIndex)));
}

IndirectFitDataPresenter::~IndirectFitDataPresenter() { observeReplace(false); }

IIndirectFitDataView const *IndirectFitDataPresenter::getView() const {
  return m_view;
}

void IndirectFitDataPresenter::setSampleWSSuffices(
    const QStringList &suffices) {
  m_view->setSampleWSSuffices(suffices);
}

void IndirectFitDataPresenter::setSampleFBSuffices(
    const QStringList &suffices) {
  m_view->setSampleFBSuffices(suffices);
}

void IndirectFitDataPresenter::setResolutionWSSuffices(
    const QStringList &suffices) {
  m_view->setResolutionWSSuffices(suffices);
}

void IndirectFitDataPresenter::setResolutionFBSuffices(
    const QStringList &suffices) {
  m_view->setResolutionFBSuffices(suffices);
}

void IndirectFitDataPresenter::setMultiInputSampleWSSuffixes() {
  if (m_addWorkspaceDialog)
    m_addWorkspaceDialog->setWSSuffices(m_view->getSampleWSSuffices());
}

void IndirectFitDataPresenter::setMultiInputSampleFBSuffixes() {
  if (m_addWorkspaceDialog)
    m_addWorkspaceDialog->setFBSuffices(m_view->getSampleFBSuffices());
}

void IndirectFitDataPresenter::setMultiInputResolutionWSSuffixes() {
  if (m_addWorkspaceDialog)
    setMultiInputResolutionWSSuffixes(m_addWorkspaceDialog.get());
}

void IndirectFitDataPresenter::setMultiInputResolutionFBSuffixes() {
  if (m_addWorkspaceDialog)
    setMultiInputResolutionFBSuffixes(m_addWorkspaceDialog.get());
}

void IndirectFitDataPresenter::setMultiInputResolutionFBSuffixes(
    IAddWorkspaceDialog *dialog) {
  UNUSED_ARG(dialog);
}

void IndirectFitDataPresenter::setMultiInputResolutionWSSuffixes(
    IAddWorkspaceDialog *dialog) {
  UNUSED_ARG(dialog);
}
void IndirectFitDataPresenter::setStartX(double startX, DatasetIndex dataIndex,
                                         WorkspaceIndex spectrumIndex) {
  m_tablePresenter->setStartX(startX, dataIndex, spectrumIndex);
  m_view->setStartX(startX);
}

void IndirectFitDataPresenter::setStartX(double startX,
                                         DatasetIndex dataIndex) {
  m_tablePresenter->setStartX(startX, dataIndex);
  m_view->setStartX(startX);
}

void IndirectFitDataPresenter::setEndX(double endX, DatasetIndex dataIndex,
                                       WorkspaceIndex spectrumIndex) {
  m_tablePresenter->setEndX(endX, dataIndex, spectrumIndex);
  m_view->setEndX(endX);
}

void IndirectFitDataPresenter::setEndX(double endX, DatasetIndex dataIndex) {
  m_tablePresenter->setEndX(endX, dataIndex);
  m_view->setEndX(endX);
}

void IndirectFitDataPresenter::setExclude(const std::string &exclude,
                                          DatasetIndex dataIndex,
                                          WorkspaceIndex spectrumIndex) {
  m_tablePresenter->setExclude(exclude, dataIndex, spectrumIndex);
}

void IndirectFitDataPresenter::setModelFromSingleData() {
  m_multipleData = m_model->clearWorkspaces();
  m_model->setFittingData(std::move(m_singleData));
  emit dataChanged();
}

void IndirectFitDataPresenter::setModelFromMultipleData() {
  m_singleData = m_model->clearWorkspaces();
  m_model->setFittingData(std::move(m_multipleData));
  emit dataChanged();
}

void IndirectFitDataPresenter::updateSpectraInTable(DatasetIndex dataIndex) {
  if (m_view->isMultipleDataTabSelected())
    m_tablePresenter->updateData(dataIndex);
}

void IndirectFitDataPresenter::updateDataInTable(DatasetIndex dataIndex) {
  if (m_tablePresenter->isTableEmpty())
    m_tablePresenter->addData(dataIndex);
  else
    m_tablePresenter->updateData(dataIndex);
}

void IndirectFitDataPresenter::setResolutionHidden(bool hide) {
  m_view->setResolutionHidden(hide);
}

void IndirectFitDataPresenter::setModelWorkspace(const QString &name) {
  observeReplace(false);
  setSingleModelData(name.toStdString());
  observeReplace(true);
}

void IndirectFitDataPresenter::loadSettings(const QSettings &settings) {
  m_view->readSettings(settings);
}

void IndirectFitDataPresenter::replaceHandle(const std::string &workspaceName,
                                             const Workspace_sptr &workspace) {
  UNUSED_ARG(workspace)
  if (m_model->hasWorkspace(workspaceName) &&
      !m_view->isMultipleDataTabSelected())
    selectReplacedWorkspace(QString::fromStdString(workspaceName));
}

DataForParameterEstimationCollection
IndirectFitDataPresenter::getDataForParameterEstimation(
    EstimationDataSelector selector) const {
  return m_model->getDataForParameterEstimation(selector);
}

void IndirectFitDataPresenter::selectReplacedWorkspace(
    const QString &workspaceName) {
  if (m_view->isSampleWorkspaceSelectorVisible()) {
    setModelWorkspace(workspaceName);
    emit dataChanged();
  } else
    m_view->setSampleWorkspaceSelectorIndex(workspaceName);
}

UserInputValidator &
IndirectFitDataPresenter::validate(UserInputValidator &validator) {
  return m_view->validate(validator);
}

void IndirectFitDataPresenter::showAddWorkspaceDialog() {
  if (!m_addWorkspaceDialog)
    m_addWorkspaceDialog = getAddWorkspaceDialog(m_view->parentWidget());
  m_addWorkspaceDialog->setWSSuffices(m_view->getSampleWSSuffices());
  m_addWorkspaceDialog->setFBSuffices(m_view->getSampleFBSuffices());
  m_addWorkspaceDialog->updateSelectedSpectra();
  //  setMultiInputSampleWSSuffixes();
  //  setMultiInputSampleFBSuffixes();
  m_addWorkspaceDialog->show();
  connect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this, SLOT(addData()));
  connect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this,
          SLOT(closeDialog()));
}

std::unique_ptr<IAddWorkspaceDialog>
IndirectFitDataPresenter::getAddWorkspaceDialog(QWidget *parent) const {
  return std::make_unique<AddWorkspaceDialog>(parent);
}

void IndirectFitDataPresenter::addData() {
  addData(m_addWorkspaceDialog.get());
}

void IndirectFitDataPresenter::closeDialog() {
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this,
             SLOT(addData()));
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this,
             SLOT(closeDialog()));
  m_addWorkspaceDialog->close();
}

void IndirectFitDataPresenter::addData(IAddWorkspaceDialog const *dialog) {
  try {
    addDataToModel(dialog);
    m_tablePresenter->addData(m_model->numberOfWorkspaces() - DatasetIndex{1});
    emit dataAdded();
    emit dataChanged();
  } catch (const std::runtime_error &ex) {
    displayWarning(ex.what());
  }
}

void IndirectFitDataPresenter::addDataToModel(
    IAddWorkspaceDialog const *dialog) {
  if (const auto indirectDialog =
          dynamic_cast<AddWorkspaceDialog const *>(dialog))
    m_model->addWorkspace(indirectDialog->workspaceName(),
                          indirectDialog->workspaceIndices());
}

void IndirectFitDataPresenter::setSingleModelData(const std::string &name) {
  m_model->clearWorkspaces();
  addModelData(name);
  auto const dataIndex = DatasetIndex{0};
  auto const spectra = m_model->getSpectra(dataIndex);
  if (!spectra.empty()) {
    auto const range = m_model->getFittingRange(dataIndex, spectra.front());
    m_view->setXRange(range);
  }
}

void IndirectFitDataPresenter::addModelData(const std::string &name) {
  try {
    m_model->addWorkspace(name);
  } catch (const std::runtime_error &ex) {
    displayWarning("Unable to load workspace:\n" + std::string(ex.what()));
  } catch (const std::invalid_argument &ex) {
    displayWarning("Invalid workspace:\n" + std::string(ex.what()));
  }
}

void IndirectFitDataPresenter::displayWarning(const std::string &warning) {
  m_view->displayWarning(warning);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
