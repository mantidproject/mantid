// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitOutputOptionsView.h"
#include "EditResultsDialog.h"
#include "FitOutputOptionsPresenter.h"

#include <QMessageBox>

namespace MantidQt::CustomInterfaces::Inelastic {

FitOutputOptionsView::FitOutputOptionsView(QWidget *parent)
    : API::MantidWidget(parent), m_editResultsDialog(), m_outputOptions(std::make_unique<Ui::FitOutputOptions>()),
      m_presenter() {
  m_outputOptions->setupUi(this);

  connect(m_outputOptions->cbGroupWorkspace, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          [=](int index) { this->notifyGroupWorkspaceChanged(m_outputOptions->cbGroupWorkspace->itemText(index)); });
  connect(m_outputOptions->pbPlot, &QPushButton::clicked, this, &FitOutputOptionsView::notifyPlotClicked);
  connect(m_outputOptions->pbPlot, &QPushButton::clicked, this, &FitOutputOptionsView::notifyPlotClicked);
  connect(m_outputOptions->pbSave, &QPushButton::clicked, this, &FitOutputOptionsView::notifySaveClicked);
  connect(m_outputOptions->pbEditResult, &QPushButton::clicked, this, &FitOutputOptionsView::handleEditResultClicked);
}

void FitOutputOptionsView::subscribePresenter(IFitOutputOptionsPresenter *presenter) { m_presenter = presenter; }

void FitOutputOptionsView::notifyGroupWorkspaceChanged(QString const &group) {
  m_presenter->handleGroupWorkspaceChanged(group.toStdString());
}

void FitOutputOptionsView::notifyPlotClicked() { m_presenter->handlePlotClicked(); }

void FitOutputOptionsView::notifySaveClicked() { m_presenter->handleSaveClicked(); }

void FitOutputOptionsView::notifyReplaceSingleFitResult() {
  m_presenter->handleReplaceSingleFitResult(m_editResultsDialog->getSelectedInputWorkspaceName(),
                                            m_editResultsDialog->getSelectedSingleFitWorkspaceName(),
                                            m_editResultsDialog->getOutputWorkspaceName());
}

void FitOutputOptionsView::setGroupWorkspaceComboBoxVisible(bool visible) {
  m_outputOptions->cbGroupWorkspace->setVisible(visible);
}

void FitOutputOptionsView::setWorkspaceComboBoxVisible(bool visible) {
  m_outputOptions->cbWorkspace->setVisible(visible);
}

void FitOutputOptionsView::clearPlotWorkspaces() { m_outputOptions->cbWorkspace->clear(); }

void FitOutputOptionsView::clearPlotTypes() { m_outputOptions->cbPlotType->clear(); }

void FitOutputOptionsView::setAvailablePlotWorkspaces(std::vector<std::string> const &workspaceNames) {
  for (auto const &name : workspaceNames)
    m_outputOptions->cbWorkspace->addItem(QString::fromStdString(name));
}

void FitOutputOptionsView::setAvailablePlotTypes(std::vector<std::string> const &parameterNames) {
  m_outputOptions->cbPlotType->addItem("All");
  for (auto const &name : parameterNames)
    m_outputOptions->cbPlotType->addItem(QString::fromStdString(name));
}

void FitOutputOptionsView::setPlotGroupWorkspaceIndex(int index) {
  m_outputOptions->cbGroupWorkspace->setCurrentIndex(index);
}

void FitOutputOptionsView::setPlotWorkspacesIndex(int index) { m_outputOptions->cbWorkspace->setCurrentIndex(index); }

void FitOutputOptionsView::setPlotTypeIndex(int index) { m_outputOptions->cbPlotType->setCurrentIndex(index); }

std::string FitOutputOptionsView::getSelectedGroupWorkspace() const {
  return m_outputOptions->cbGroupWorkspace->currentText().toStdString();
}

std::string FitOutputOptionsView::getSelectedWorkspace() const {
  return m_outputOptions->cbWorkspace->currentText().toStdString();
}

std::string FitOutputOptionsView::getSelectedPlotType() const {
  return m_outputOptions->cbPlotType->currentText().toStdString();
}

void FitOutputOptionsView::setPlotText(std::string const &text) {
  m_outputOptions->pbPlot->setText(QString::fromStdString(text));
}

void FitOutputOptionsView::setSaveText(std::string const &text) {
  m_outputOptions->pbSave->setText(QString::fromStdString(text));
}

void FitOutputOptionsView::setPlotExtraOptionsEnabled(bool enable) {
  m_outputOptions->cbGroupWorkspace->setEnabled(enable);
  m_outputOptions->cbWorkspace->setEnabled(enable);
}

void FitOutputOptionsView::setPlotEnabled(bool enable) {
  m_outputOptions->pbPlot->setEnabled(enable);
  m_outputOptions->cbPlotType->setEnabled(enable);
}

void FitOutputOptionsView::setEditResultEnabled(bool enable) { m_outputOptions->pbEditResult->setEnabled(enable); }

void FitOutputOptionsView::setSaveEnabled(bool enable) { m_outputOptions->pbSave->setEnabled(enable); }

void FitOutputOptionsView::setEditResultVisible(bool visible) { m_outputOptions->pbEditResult->setVisible(visible); }

void FitOutputOptionsView::handleEditResultClicked() {
  m_editResultsDialog = new EditResultsDialog(this);
  m_editResultsDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_editResultsDialog->setWorkspaceSelectorSuffices({"_Result"});
  m_editResultsDialog->show();
  connect(m_editResultsDialog, &EditResultsDialog::replaceSingleFitResult, this,
          &FitOutputOptionsView::notifyReplaceSingleFitResult);
}

void FitOutputOptionsView::displayWarning(std::string const &message) {
  QMessageBox::warning(parentWidget(), "MantidPlot - Warning", QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces::Inelastic
