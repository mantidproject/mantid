// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "JumpFitDataPresenter.h"
#include "JumpFitDataTablePresenter.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

JumpFitDataPresenter::JumpFitDataPresenter(
    JumpFitModel *model, IIndirectFitDataView *view, QComboBox *cbParameterType,
    QComboBox *cbParameter, QLabel *lbParameterType, QLabel *lbParameter)
    : IndirectFitDataPresenter(model, view,
                               std::make_unique<JumpFitDataTablePresenter>(
                                   model, view->getDataTable())),
      m_activeParameterType("Width"), m_dataIndex{0},
      m_cbParameterType(cbParameterType), m_cbParameter(cbParameter),
      m_lbParameterType(lbParameterType), m_lbParameter(lbParameter),
      m_jumpModel(model) {
  connect(view, SIGNAL(singleDataViewSelected()), this,
          SLOT(showParameterComboBoxes()));
  connect(view, SIGNAL(multipleDataViewSelected()), this,
          SLOT(hideParameterComboBoxes()));

  connect(this, SIGNAL(requestedAddWorkspaceDialog()), this,
          SLOT(updateActiveDataIndex()));

  connect(cbParameterType, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(setParameterLabel(const QString &)));
  connect(cbParameterType, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(updateAvailableParameters(QString const &)));
  connect(cbParameterType, SIGNAL(currentIndexChanged(const QString &)), this,
          SIGNAL(dataChanged()));
  connect(cbParameter, SIGNAL(currentIndexChanged(int)), this,
          SLOT(setSingleModelSpectrum(int)));
  connect(cbParameter, SIGNAL(currentIndexChanged(int)), this,
          SIGNAL(dataChanged()));

  connect(view, SIGNAL(sampleLoaded(const QString &)), this,
          SLOT(updateAvailableParameterTypes()));
  connect(view, SIGNAL(sampleLoaded(const QString &)), this,
          SLOT(updateAvailableParameters()));
  connect(view, SIGNAL(sampleLoaded(const QString &)), this,
          SLOT(updateParameterSelectionEnabled()));
  connect(view, SIGNAL(sampleLoaded(const QString &)), this,
          SIGNAL(updateAvailableFitTypes()));

  updateParameterSelectionEnabled();
}

void JumpFitDataPresenter::hideParameterComboBoxes() {
  m_cbParameter->hide();
  m_cbParameterType->hide();
  m_lbParameter->hide();
  m_lbParameterType->hide();
}

void JumpFitDataPresenter::showParameterComboBoxes() {
  m_cbParameter->show();
  m_cbParameterType->show();
  m_lbParameter->show();
  m_lbParameterType->show();
}

void JumpFitDataPresenter::setActiveParameterType(const std::string &type) {
  m_activeParameterType = type;
}

void JumpFitDataPresenter::updateActiveDataIndex() {
  m_dataIndex = m_jumpModel->numberOfWorkspaces();
}

void JumpFitDataPresenter::updateAvailableParameters() {
  updateAvailableParameters(m_cbParameterType->currentText());
}

void JumpFitDataPresenter::updateAvailableParameters(const QString &type) {
  if (type == "Width")
    setAvailableParameters(m_jumpModel->getWidths(DatasetIndex{0}));
  else if (type == "EISF")
    setAvailableParameters(m_jumpModel->getEISF(DatasetIndex{0}));
  else
    setAvailableParameters({});

  if (!type.isEmpty())
    setSingleModelSpectrum(m_cbParameter->currentIndex());
}

void JumpFitDataPresenter::updateAvailableParameterTypes() {
  MantidQt::API::SignalBlocker blocker(m_cbParameterType);
  m_cbParameterType->clear();
  for (const auto &type : getParameterTypes(m_dataIndex))
    m_cbParameterType->addItem(QString::fromStdString(type));
}

void JumpFitDataPresenter::updateParameterSelectionEnabled() {
  const auto enabled = m_jumpModel->numberOfWorkspaces().value > 0;
  m_cbParameter->setEnabled(enabled);
  m_cbParameterType->setEnabled(enabled);
  m_lbParameter->setEnabled(enabled);
}

void JumpFitDataPresenter::setAvailableParameters(
    const std::vector<std::string> &parameters) {
  MantidQt::API::SignalBlocker blocker(m_cbParameter);
  m_cbParameter->clear();
  for (const auto &parameter : parameters)
    m_cbParameter->addItem(QString::fromStdString(parameter));
}

void JumpFitDataPresenter::setParameterLabel(const QString &parameter) {
  m_lbParameter->setText(parameter + ":");
}

void JumpFitDataPresenter::setDialogParameterNames(
    JumpFitAddWorkspaceDialog *dialog, const std::string &workspace) {
  try {
    addWorkspace(m_jumpModel, workspace);
    dialog->enableParameterSelection();
  } catch (const std::invalid_argument &) {
    dialog->disableParameterSelection();
  }
  updateParameterTypes(dialog);
  updateParameterOptions(dialog);
}

void JumpFitDataPresenter::dialogParameterTypeUpdated(
    JumpFitAddWorkspaceDialog *dialog, const std::string &type) {
  setActiveParameterType(type);
  updateParameterOptions(dialog);
}

void JumpFitDataPresenter::updateParameterOptions(
    JumpFitAddWorkspaceDialog *dialog) {
  if (m_activeParameterType == "Width")
    dialog->setParameterNames(m_jumpModel->getWidths(m_dataIndex));
  else if (m_activeParameterType == "EISF")
    dialog->setParameterNames(m_jumpModel->getEISF(m_dataIndex));
  else
    dialog->setParameterNames({});
}

void JumpFitDataPresenter::updateParameterTypes(
    JumpFitAddWorkspaceDialog *dialog) {
  dialog->setParameterTypes(getParameterTypes(m_dataIndex));
}

std::vector<std::string>
JumpFitDataPresenter::getParameterTypes(DatasetIndex dataIndex) const {
  std::vector<std::string> types;
  if (!m_jumpModel->zeroWidths(dataIndex))
    types.emplace_back("Width");
  if (!m_jumpModel->zeroEISF(dataIndex))
    types.emplace_back("EISF");
  return types;
}

void JumpFitDataPresenter::addWorkspace(IndirectFittingModel *model,
                                        const std::string &name) {
  if (model->numberOfWorkspaces() > m_dataIndex)
    model->removeWorkspace(m_dataIndex);
  model->addWorkspace(name);
}

void JumpFitDataPresenter::addDataToModel(IAddWorkspaceDialog const *dialog) {
  if (const auto jumpDialog =
          dynamic_cast<JumpFitAddWorkspaceDialog const *>(dialog)) {
    setModelSpectrum(jumpDialog->parameterNameIndex());
    updateActiveDataIndex();
  }
}

void JumpFitDataPresenter::setSingleModelSpectrum(int parameterIndex) {
  auto index = static_cast<std::size_t>(parameterIndex);
  if (m_cbParameterType->currentIndex() == 0)
    m_jumpModel->setActiveWidth(index, DatasetIndex{0});
  else
    m_jumpModel->setActiveEISF(index, DatasetIndex{0});
}

void JumpFitDataPresenter::setModelSpectrum(int index) {
  if (index < 0)
    throw std::runtime_error("No valid parameter was selected.");
  else if (m_activeParameterType == "Width")
    m_jumpModel->setActiveWidth(static_cast<std::size_t>(index), m_dataIndex);
  else
    m_jumpModel->setActiveEISF(static_cast<std::size_t>(index), m_dataIndex);
}

void JumpFitDataPresenter::closeDialog() {
  if (m_jumpModel->numberOfWorkspaces() > m_dataIndex)
    m_jumpModel->removeWorkspace(m_dataIndex);
  IndirectFitDataPresenter::closeDialog();
}

std::unique_ptr<IAddWorkspaceDialog>
JumpFitDataPresenter::getAddWorkspaceDialog(QWidget *parent) const {
  auto dialog = std::make_unique<JumpFitAddWorkspaceDialog>(parent);
  connect(dialog.get(),
          SIGNAL(workspaceChanged(JumpFitAddWorkspaceDialog *,
                                  const std::string &)),
          this,
          SLOT(setDialogParameterNames(JumpFitAddWorkspaceDialog *,
                                       const std::string &)));
  connect(dialog.get(),
          SIGNAL(parameterTypeChanged(JumpFitAddWorkspaceDialog *,
                                      const std::string &)),
          this,
          SLOT(dialogParameterTypeUpdated(JumpFitAddWorkspaceDialog *,
                                          const std::string &)));
  return std::move(dialog);
}

void JumpFitDataPresenter::setMultiInputResolutionFBSuffixes(
    IAddWorkspaceDialog *dialog) {
  UNUSED_ARG(dialog);
}

void JumpFitDataPresenter::setMultiInputResolutionWSSuffixes(
    IAddWorkspaceDialog *dialog) {
  UNUSED_ARG(dialog);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
