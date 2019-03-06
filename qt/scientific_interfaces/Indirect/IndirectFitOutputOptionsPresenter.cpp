// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsPresenter.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputOptionsPresenter::IndirectFitOutputOptionsPresenter(
    IIndirectFitOutputOptionsView *view)
    : QObject(nullptr),
      m_model(std::make_unique<IndirectFitOutputOptionsModel>()), m_view(view) {
  setUpPresenter();
}

IndirectFitOutputOptionsPresenter::IndirectFitOutputOptionsPresenter(
    IIndirectFitOutputOptionsModel *model, IIndirectFitOutputOptionsView *view)
    : QObject(nullptr), m_model(model), m_view(view) {
  setUpPresenter();
}

IndirectFitOutputOptionsPresenter::~IndirectFitOutputOptionsPresenter() {}

void IndirectFitOutputOptionsPresenter::setUpPresenter() {
  setMultiWorkspaceOptionsVisible(false);

  connect(m_view, SIGNAL(groupWorkspaceChanged(std::string const &)), this,
          SLOT(setAvailablePlotOptions(std::string const &)));

  connect(m_view, SIGNAL(plotClicked()), this, SLOT(plotResult()));
  connect(m_view, SIGNAL(plotClicked()), this, SIGNAL(plotSpectra()));
  connect(m_view, SIGNAL(saveClicked()), this, SLOT(saveResult()));
  connect(m_view, SIGNAL(editResultClicked()), this, SLOT(editResult()));
}

void IndirectFitOutputOptionsPresenter::setMultiWorkspaceOptionsVisible(
    bool visible) {
  m_view->setGroupWorkspaceComboBoxVisible(visible);
  m_view->setPlotGroupWorkspaceIndex(0);
  m_view->setWorkspaceComboBoxVisible(false);
}

void IndirectFitOutputOptionsPresenter::setAvailablePlotOptions(
    std::string const &selectedGroup) {
  auto const resultSelected = m_model->isResultGroupSelected(selectedGroup);
  setPlotTypes(selectedGroup);
  m_view->setWorkspaceComboBoxVisible(!resultSelected);
  m_view->setPlotEnabled(resultSelected ? isResultGroupPlottable()
                                        : isPDFGroupPlottable());
}

void IndirectFitOutputOptionsPresenter::setResultWorkspace(
    WorkspaceGroup_sptr groupWorkspace) {
  m_model->setResultWorkspace(groupWorkspace);
}

void IndirectFitOutputOptionsPresenter::setPDFWorkspace(
    WorkspaceGroup_sptr groupWorkspace) {
  m_model->setPDFWorkspace(groupWorkspace);
}

void IndirectFitOutputOptionsPresenter::setPlotWorkspaces() {
  m_view->clearPlotWorkspaces();
  auto const workspaceNames = m_model->getPDFWorkspaceNames();
  if (!workspaceNames.empty()) {
    m_view->setAvailablePlotWorkspaces(workspaceNames);
    m_view->setPlotWorkspacesIndex(0);
  }
}

void IndirectFitOutputOptionsPresenter::setPlotTypes(
    std::string const &selectedGroup) {
  m_view->clearPlotTypes();
  auto const parameterNames = m_model->getWorkspaceParameters(selectedGroup);
  if (!parameterNames.empty()) {
    m_view->setAvailablePlotTypes(parameterNames);
    m_view->setPlotTypeIndex(0);
  }
}

void IndirectFitOutputOptionsPresenter::removePDFWorkspace() {
  m_model->removePDFWorkspace();
}

void IndirectFitOutputOptionsPresenter::plotResult() {
  setPlotting(true);
  try {
    plotResult(m_view->getSelectedGroupWorkspace());
  } catch (std::runtime_error const &ex) {
    displayWarning(ex.what());
  }
}

void IndirectFitOutputOptionsPresenter::plotResult(
    std::string const &selectedGroup) {
  if (m_model->isResultGroupSelected(selectedGroup))
    m_model->plotResult(m_view->getSelectedPlotType());
  else
    m_model->plotPDF(m_view->getSelectedWorkspace(),
                     m_view->getSelectedPlotType());
}

void IndirectFitOutputOptionsPresenter::saveResult() {
  setSaving(true);
  try {
    m_model->saveResult();
  } catch (std::runtime_error const &ex) {
    displayWarning(ex.what());
  }
  setSaving(false);
}

bool IndirectFitOutputOptionsPresenter::isResultGroupPlottable() {
  return m_model->isResultGroupPlottable();
}

bool IndirectFitOutputOptionsPresenter::isPDFGroupPlottable() {
  return m_model->isPDFGroupPlottable();
}

void IndirectFitOutputOptionsPresenter::setPlotting(bool plotting) {
  m_view->setPlotText(plotting ? "Plotting..." : "Plot");
  m_view->setPlotExtraOptionsEnabled(!plotting);
  setPlotEnabled(!plotting);
  setEditResultEnabled(!plotting);
  setSaveEnabled(!plotting);
}

void IndirectFitOutputOptionsPresenter::setSaving(bool saving) {
  m_view->setSaveText(saving ? "Saving..." : "Save Result");
  setPlotEnabled(!saving);
  setEditResultEnabled(!saving);
  setSaveEnabled(!saving);
}

void IndirectFitOutputOptionsPresenter::setPlotEnabled(bool enable) {
  m_view->setPlotEnabled(enable);
}

void IndirectFitOutputOptionsPresenter::setEditResultEnabled(bool enable) {
  m_view->setEditResultEnabled(enable);
}

void IndirectFitOutputOptionsPresenter::setSaveEnabled(bool enable) {
  m_view->setSaveEnabled(enable);
}

void IndirectFitOutputOptionsPresenter::clearSpectraToPlot() {
  m_model->clearSpectraToPlot();
}

std::vector<SpectrumToPlot>
IndirectFitOutputOptionsPresenter::getSpectraToPlot() const {
  return m_model->getSpectraToPlot();
}

void IndirectFitOutputOptionsPresenter::setEditResultVisible(bool visible) {
  m_view->setEditResultVisible(visible);
}

void IndirectFitOutputOptionsPresenter::editResult() {
  m_editResultsDialog = getEditResultsDialog(m_view->parentWidget());
  m_editResultsDialog->setWorkspaceSelectorSuffices({"_Result"});
  m_editResultsDialog->show();
  connect(m_editResultsDialog.get(), SIGNAL(replaceSingleFitResult()), this,
          SLOT(replaceSingleFitResult()));
  connect(m_editResultsDialog.get(), SIGNAL(closeDialog()), this,
          SLOT(closeEditResultDialog()));
}

std::unique_ptr<IndirectEditResultsDialog>
IndirectFitOutputOptionsPresenter::getEditResultsDialog(QWidget *parent) const {
  return Mantid::Kernel::make_unique<IndirectEditResultsDialog>(parent);
}

void IndirectFitOutputOptionsPresenter::replaceSingleFitResult() {
  auto const inputName = m_editResultsDialog->getSelectedInputWorkspaceName();
  auto const singleBinName =
      m_editResultsDialog->getSelectedSingleFitWorkspaceName();
  auto const outputName = m_editResultsDialog->getOutputWorkspaceName();

  setEditingResult(true);
  replaceSingleFitResult(inputName, singleBinName, outputName);
  setEditingResult(false);
}

void IndirectFitOutputOptionsPresenter::replaceSingleFitResult(
    std::string const &inputName, std::string const &singleBinName,
    std::string const &outputName) {
  try {
    m_model->replaceFitResult(inputName, singleBinName, outputName);
  } catch (std::exception const &ex) {
    m_view->displayWarning(ex.what());
  }
}

void IndirectFitOutputOptionsPresenter::setEditingResult(bool editing) {
  m_editResultsDialog->setReplaceFitResultText(editing ? "Processing..."
                                                       : "Replace Fit Result");
  m_editResultsDialog->setReplaceFitResultEnabled(!editing);
  setPlotEnabled(!editing);
  setEditResultEnabled(!editing);
  setSaveEnabled(!editing);
}

void IndirectFitOutputOptionsPresenter::closeEditResultDialog() {
  disconnect(m_editResultsDialog.get(), SIGNAL(replaceSingleFitResult()), this,
             SLOT(replaceSingleFitResult()));
  disconnect(m_editResultsDialog.get(), SIGNAL(closeDialog()), this,
             SLOT(closeEditResultDialog()));
  m_editResultsDialog->close();
}

void IndirectFitOutputOptionsPresenter::displayWarning(
    std::string const &message) {
  m_view->displayWarning(message);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
