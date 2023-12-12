// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsView.h"
#include "IndirectEditResultsDialog.h"
#include "IndirectFitOutputOptionsPresenter.h"

#include <QMessageBox>

namespace MantidQt::CustomInterfaces::IDA {

IndirectFitOutputOptionsView::IndirectFitOutputOptionsView(QWidget *parent)
    : API::MantidWidget(parent), m_editResultsDialog(),
      m_outputOptions(std::make_unique<Ui::IndirectFitOutputOptions>()), m_presenter() {
  m_outputOptions->setupUi(this);

  connect(m_outputOptions->cbGroupWorkspace, SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(notifyGroupWorkspaceChanged(QString const &)));

  connect(m_outputOptions->pbPlot, SIGNAL(clicked()), this, SLOT(notifyPlotClicked()));
  connect(m_outputOptions->pbSave, SIGNAL(clicked()), this, SLOT(notifySaveClicked()));
  connect(m_outputOptions->pbEditResult, SIGNAL(clicked()), this, SLOT(handleEditResultClicked()));
}

void IndirectFitOutputOptionsView::subscribePresenter(IIndirectFitOutputOptionsPresenter *presenter) {
  m_presenter = presenter;
}

void IndirectFitOutputOptionsView::notifyGroupWorkspaceChanged(QString const &group) {
  m_presenter->handleGroupWorkspaceChanged(group.toStdString());
}

void IndirectFitOutputOptionsView::notifyPlotClicked() { m_presenter->handlePlotClicked(); }

void IndirectFitOutputOptionsView::notifySaveClicked() { m_presenter->handleSaveClicked(); }

void IndirectFitOutputOptionsView::notifyReplaceSingleFitResult() {
  m_presenter->handleReplaceSingleFitResult(m_editResultsDialog->getSelectedInputWorkspaceName(),
                                            m_editResultsDialog->getSelectedSingleFitWorkspaceName(),
                                            m_editResultsDialog->getOutputWorkspaceName());
}

void IndirectFitOutputOptionsView::setGroupWorkspaceComboBoxVisible(bool visible) {
  m_outputOptions->cbGroupWorkspace->setVisible(visible);
}

void IndirectFitOutputOptionsView::setWorkspaceComboBoxVisible(bool visible) {
  m_outputOptions->cbWorkspace->setVisible(visible);
}

void IndirectFitOutputOptionsView::clearPlotWorkspaces() { m_outputOptions->cbWorkspace->clear(); }

void IndirectFitOutputOptionsView::clearPlotTypes() { m_outputOptions->cbPlotType->clear(); }

void IndirectFitOutputOptionsView::setAvailablePlotWorkspaces(std::vector<std::string> const &workspaceNames) {
  for (auto const &name : workspaceNames)
    m_outputOptions->cbWorkspace->addItem(QString::fromStdString(name));
}

void IndirectFitOutputOptionsView::setAvailablePlotTypes(std::vector<std::string> const &parameterNames) {
  m_outputOptions->cbPlotType->addItem("All");
  for (auto const &name : parameterNames)
    m_outputOptions->cbPlotType->addItem(QString::fromStdString(name));
}

void IndirectFitOutputOptionsView::setPlotGroupWorkspaceIndex(int index) {
  m_outputOptions->cbGroupWorkspace->setCurrentIndex(index);
}

void IndirectFitOutputOptionsView::setPlotWorkspacesIndex(int index) {
  m_outputOptions->cbWorkspace->setCurrentIndex(index);
}

void IndirectFitOutputOptionsView::setPlotTypeIndex(int index) { m_outputOptions->cbPlotType->setCurrentIndex(index); }

std::string IndirectFitOutputOptionsView::getSelectedGroupWorkspace() const {
  return m_outputOptions->cbGroupWorkspace->currentText().toStdString();
}

std::string IndirectFitOutputOptionsView::getSelectedWorkspace() const {
  return m_outputOptions->cbWorkspace->currentText().toStdString();
}

std::string IndirectFitOutputOptionsView::getSelectedPlotType() const {
  return m_outputOptions->cbPlotType->currentText().toStdString();
}

void IndirectFitOutputOptionsView::setPlotText(std::string const &text) {
  m_outputOptions->pbPlot->setText(QString::fromStdString(text));
}

void IndirectFitOutputOptionsView::setSaveText(std::string const &text) {
  m_outputOptions->pbSave->setText(QString::fromStdString(text));
}

void IndirectFitOutputOptionsView::setPlotExtraOptionsEnabled(bool enable) {
  m_outputOptions->cbGroupWorkspace->setEnabled(enable);
  m_outputOptions->cbWorkspace->setEnabled(enable);
}

void IndirectFitOutputOptionsView::setPlotEnabled(bool enable) {
  m_outputOptions->pbPlot->setEnabled(enable);
  m_outputOptions->cbPlotType->setEnabled(enable);
}

void IndirectFitOutputOptionsView::setEditResultEnabled(bool enable) {
  m_outputOptions->pbEditResult->setEnabled(enable);
}

void IndirectFitOutputOptionsView::setSaveEnabled(bool enable) { m_outputOptions->pbSave->setEnabled(enable); }

void IndirectFitOutputOptionsView::setEditResultVisible(bool visible) {
  m_outputOptions->pbEditResult->setVisible(visible);
}

void IndirectFitOutputOptionsView::handleEditResultClicked() {
  m_editResultsDialog = new IndirectEditResultsDialog(this);
  m_editResultsDialog->setWorkspaceSelectorSuffices({"_Result"});
  m_editResultsDialog->show();
  connect(m_editResultsDialog, SIGNAL(replaceSingleFitResult()), this, SLOT(notifyReplaceSingleFitResult()));
  connect(m_editResultsDialog, SIGNAL(closeDialog()), this, SLOT(handleCloseEditResultDialog()));
}

void IndirectFitOutputOptionsView::handleCloseEditResultDialog() {
  disconnect(m_editResultsDialog, SIGNAL(replaceSingleFitResult()), this, SLOT(notifyReplaceSingleFitResult()));
  disconnect(m_editResultsDialog, SIGNAL(closeDialog()), this, SLOT(closeEditResultDialog()));
  m_editResultsDialog->close();
}

void IndirectFitOutputOptionsView::displayWarning(std::string const &message) {
  QMessageBox::warning(parentWidget(), "MantidPlot - Warning", QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces::IDA
