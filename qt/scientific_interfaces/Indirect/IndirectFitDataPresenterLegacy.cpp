// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitDataPresenterLegacy.h"
#include "IndirectAddWorkspaceDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitDataPresenterLegacy::IndirectFitDataPresenterLegacy(
    IndirectFittingModelLegacy *model, IIndirectFitDataViewLegacy *view)
    : IndirectFitDataPresenterLegacy(
          model, view,
          std::make_unique<IndirectDataTablePresenterLegacy>(
              model, view->getDataTable())) {}

IndirectFitDataPresenterLegacy::IndirectFitDataPresenterLegacy(
    IndirectFittingModelLegacy *model, IIndirectFitDataViewLegacy *view,
    std::unique_ptr<IndirectDataTablePresenterLegacy> tablePresenter)
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

  connect(m_tablePresenter.get(),
          SIGNAL(startXChanged(double, std::size_t, std::size_t)), this,
          SIGNAL(startXChanged(double, std::size_t, std::size_t)));
  connect(m_tablePresenter.get(),
          SIGNAL(endXChanged(double, std::size_t, std::size_t)), this,
          SIGNAL(endXChanged(double, std::size_t, std::size_t)));
  connect(m_tablePresenter.get(),
          SIGNAL(excludeRegionChanged(const std::string &, std::size_t,
                                      std::size_t)),
          this,
          SIGNAL(excludeRegionChanged(const std::string &, std::size_t,
                                      std::size_t)));
}

IndirectFitDataPresenterLegacy::~IndirectFitDataPresenterLegacy() {
  observeReplace(false);
}

IIndirectFitDataViewLegacy const *
IndirectFitDataPresenterLegacy::getView() const {
  return m_view;
}

void IndirectFitDataPresenterLegacy::setSampleWSSuffices(
    const QStringList &suffices) {
  m_view->setSampleWSSuffices(suffices);
}

void IndirectFitDataPresenterLegacy::setSampleFBSuffices(
    const QStringList &suffices) {
  m_view->setSampleFBSuffices(suffices);
}

void IndirectFitDataPresenterLegacy::setResolutionWSSuffices(
    const QStringList &suffices) {
  m_view->setResolutionWSSuffices(suffices);
}

void IndirectFitDataPresenterLegacy::setResolutionFBSuffices(
    const QStringList &suffices) {
  m_view->setResolutionFBSuffices(suffices);
}

void IndirectFitDataPresenterLegacy::setMultiInputSampleWSSuffixes() {
  if (m_addWorkspaceDialog)
    m_addWorkspaceDialog->setWSSuffices(m_view->getSampleWSSuffices());
}

void IndirectFitDataPresenterLegacy::setMultiInputSampleFBSuffixes() {
  if (m_addWorkspaceDialog)
    m_addWorkspaceDialog->setFBSuffices(m_view->getSampleFBSuffices());
}

void IndirectFitDataPresenterLegacy::setMultiInputResolutionWSSuffixes() {
  if (m_addWorkspaceDialog)
    setMultiInputResolutionWSSuffixes(m_addWorkspaceDialog.get());
}

void IndirectFitDataPresenterLegacy::setMultiInputResolutionFBSuffixes() {
  if (m_addWorkspaceDialog)
    setMultiInputResolutionFBSuffixes(m_addWorkspaceDialog.get());
}

void IndirectFitDataPresenterLegacy::setMultiInputResolutionFBSuffixes(
    IAddWorkspaceDialog *dialog) {
  UNUSED_ARG(dialog);
}

void IndirectFitDataPresenterLegacy::setMultiInputResolutionWSSuffixes(
    IAddWorkspaceDialog *dialog) {
  UNUSED_ARG(dialog);
}

void IndirectFitDataPresenterLegacy::setStartX(double startX,
                                               std::size_t dataIndex,
                                               int spectrumIndex) {
  m_tablePresenter->setStartX(startX, dataIndex, spectrumIndex);
}

void IndirectFitDataPresenterLegacy::setEndX(double endX, std::size_t dataIndex,
                                             int spectrumIndex) {
  m_tablePresenter->setEndX(endX, dataIndex, spectrumIndex);
}

void IndirectFitDataPresenterLegacy::setExclude(const std::string &exclude,
                                                std::size_t dataIndex,
                                                int spectrumIndex) {
  m_tablePresenter->setExclude(exclude, dataIndex, spectrumIndex);
}

void IndirectFitDataPresenterLegacy::setModelFromSingleData() {
  m_multipleData = m_model->clearWorkspaces();
  m_model->setFittingData(std::move(m_singleData));
  emit dataChanged();
}

void IndirectFitDataPresenterLegacy::setModelFromMultipleData() {
  m_singleData = m_model->clearWorkspaces();
  m_model->setFittingData(std::move(m_multipleData));
  emit dataChanged();
}

void IndirectFitDataPresenterLegacy::updateSpectraInTable(
    std::size_t dataIndex) {
  if (m_view->isMultipleDataTabSelected())
    m_tablePresenter->updateData(dataIndex);
}

void IndirectFitDataPresenterLegacy::updateDataInTable(std::size_t dataIndex) {
  if (m_tablePresenter->isTableEmpty())
    m_tablePresenter->addData(dataIndex);
  else
    m_tablePresenter->updateData(dataIndex);
}

void IndirectFitDataPresenterLegacy::setResolutionHidden(bool hide) {
  m_view->setResolutionHidden(hide);
}

void IndirectFitDataPresenterLegacy::setModelWorkspace(const QString &name) {
  observeReplace(false);
  setSingleModelData(name.toStdString());
  observeReplace(true);
}

void IndirectFitDataPresenterLegacy::loadSettings(const QSettings &settings) {
  m_view->readSettings(settings);
}

void IndirectFitDataPresenterLegacy::replaceHandle(
    const std::string &workspaceName, const Workspace_sptr &workspace) {
  UNUSED_ARG(workspace)
  if (m_model->hasWorkspace(workspaceName) &&
      !m_view->isMultipleDataTabSelected())
    selectReplacedWorkspace(QString::fromStdString(workspaceName));
}

void IndirectFitDataPresenterLegacy::selectReplacedWorkspace(
    const QString &workspaceName) {
  if (m_view->isSampleWorkspaceSelectorVisible()) {
    setModelWorkspace(workspaceName);
    emit dataChanged();
  } else
    m_view->setSampleWorkspaceSelectorIndex(workspaceName);
}

UserInputValidator &
IndirectFitDataPresenterLegacy::validate(UserInputValidator &validator) {
  return m_view->validate(validator);
}

void IndirectFitDataPresenterLegacy::showAddWorkspaceDialog() {
  if (!m_addWorkspaceDialog)
    m_addWorkspaceDialog = getAddWorkspaceDialog(m_view->parentWidget());
  m_addWorkspaceDialog->updateSelectedSpectra();
  setMultiInputSampleWSSuffixes();
  setMultiInputSampleFBSuffixes();
  m_addWorkspaceDialog->show();
  connect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this, SLOT(addData()));
  connect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this,
          SLOT(closeDialog()));
}

std::unique_ptr<IAddWorkspaceDialog>
IndirectFitDataPresenterLegacy::getAddWorkspaceDialog(QWidget *parent) const {
  return std::make_unique<AddWorkspaceDialog>(parent);
}

void IndirectFitDataPresenterLegacy::addData() {
  addData(m_addWorkspaceDialog.get());
}

void IndirectFitDataPresenterLegacy::closeDialog() {
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(addData()), this,
             SLOT(addData()));
  disconnect(m_addWorkspaceDialog.get(), SIGNAL(closeDialog()), this,
             SLOT(closeDialog()));
  m_addWorkspaceDialog->close();
}

void IndirectFitDataPresenterLegacy::addData(
    IAddWorkspaceDialog const *dialog) {
  try {
    addDataToModel(dialog);
    m_tablePresenter->addData(m_model->numberOfWorkspaces() - 1);
    emit dataAdded();
    emit dataChanged();
  } catch (const std::runtime_error &ex) {
    displayWarning(ex.what());
  }
}

void IndirectFitDataPresenterLegacy::addDataToModel(
    IAddWorkspaceDialog const *dialog) {
  if (const auto indirectDialog =
          dynamic_cast<AddWorkspaceDialog const *>(dialog))
    m_model->addWorkspace(indirectDialog->workspaceName(),
                          indirectDialog->workspaceIndices());
}

void IndirectFitDataPresenterLegacy::setSingleModelData(
    const std::string &name) {
  m_model->clearWorkspaces();
  addModelData(name);
}

void IndirectFitDataPresenterLegacy::addModelData(const std::string &name) {
  try {
    m_model->addWorkspace(name);
  } catch (const std::runtime_error &ex) {
    displayWarning("Unable to load workspace:\n" + std::string(ex.what()));
  } catch (const std::invalid_argument &ex) {
    displayWarning("Invalid workspace:\n" + std::string(ex.what()));
  }
}

void IndirectFitDataPresenterLegacy::displayWarning(
    const std::string &warning) {
  m_view->displayWarning(warning);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
