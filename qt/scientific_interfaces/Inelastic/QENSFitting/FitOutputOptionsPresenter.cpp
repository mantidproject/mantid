// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitOutputOptionsPresenter.h"
#include "FitTab.h"

#include <utility>

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces::Inelastic {

FitOutputOptionsPresenter::FitOutputOptionsPresenter(IFitTab *tab, IFitOutputOptionsView *view,
                                                     std::unique_ptr<IFitOutputOptionsModel> model,
                                                     std::unique_ptr<Widgets::MplCpp::IExternalPlotter> plotter)
    : m_tab(tab), m_view(view), m_model(std::move(model)), m_plotter(std::move(plotter)) {
  setMultiWorkspaceOptionsVisible(false);
  m_view->subscribePresenter(this);
}

void FitOutputOptionsPresenter::setMultiWorkspaceOptionsVisible(bool visible) {
  m_view->setGroupWorkspaceComboBoxVisible(visible);
  m_view->setPlotGroupWorkspaceIndex(0);
  m_view->setWorkspaceComboBoxVisible(false);
}

void FitOutputOptionsPresenter::handleGroupWorkspaceChanged(std::string const &selectedGroup) {
  auto const resultSelected = m_model->isResultGroupSelected(selectedGroup);
  setPlotTypes(selectedGroup);
  m_view->setWorkspaceComboBoxVisible(!resultSelected);
  m_view->setPlotEnabled(isSelectedGroupPlottable());
}

void FitOutputOptionsPresenter::setResultWorkspace(WorkspaceGroup_sptr groupWorkspace) {
  m_model->setResultWorkspace(std::move(groupWorkspace));
}

void FitOutputOptionsPresenter::setPDFWorkspace(WorkspaceGroup_sptr groupWorkspace) {
  m_model->setPDFWorkspace(std::move(groupWorkspace));
}

void FitOutputOptionsPresenter::setPlotWorkspaces() {
  m_view->clearPlotWorkspaces();
  auto const workspaceNames = m_model->getPDFWorkspaceNames();
  if (!workspaceNames.empty()) {
    m_view->setAvailablePlotWorkspaces(workspaceNames);
    m_view->setPlotWorkspacesIndex(0);
  }
}

void FitOutputOptionsPresenter::setPlotTypes(std::string const &selectedGroup) {
  m_view->clearPlotTypes();
  auto const parameterNames = m_model->getWorkspaceParameters(selectedGroup);
  if (!parameterNames.empty()) {
    m_view->setAvailablePlotTypes(parameterNames);
    m_view->setPlotTypeIndex(0);
  }
}

void FitOutputOptionsPresenter::removePDFWorkspace() { m_model->removePDFWorkspace(); }

void FitOutputOptionsPresenter::handlePlotClicked() {
  setPlotting(true);
  try {
    plotResult(m_view->getSelectedGroupWorkspace());
    m_tab->handlePlotSelectedSpectra();
  } catch (std::runtime_error const &ex) {
    displayWarning(ex.what());
  }
}

void FitOutputOptionsPresenter::plotResult(std::string const &selectedGroup) {
  if (m_model->isResultGroupSelected(selectedGroup))
    m_model->plotResult(m_view->getSelectedPlotType());
  else
    m_model->plotPDF(m_view->getSelectedWorkspace(), m_view->getSelectedPlotType());
}

void FitOutputOptionsPresenter::handleSaveClicked() {
  setSaving(true);
  try {
    m_model->saveResult();
  } catch (std::runtime_error const &ex) {
    displayWarning(ex.what());
  }
  setSaving(false);
}

bool FitOutputOptionsPresenter::isSelectedGroupPlottable() const {
  return m_model->isSelectedGroupPlottable(m_view->getSelectedGroupWorkspace());
}

void FitOutputOptionsPresenter::setPlotting(bool plotting) {
  m_view->setPlotText(plotting ? "Plotting..." : "Plot");
  m_view->setPlotExtraOptionsEnabled(!plotting);
  setPlotEnabled(!plotting);
  setEditResultEnabled(!plotting);
  setSaveEnabled(!plotting);
}

void FitOutputOptionsPresenter::setSaving(bool saving) {
  m_view->setSaveText(saving ? "Saving..." : "Save Result");
  setPlotEnabled(!saving);
  setEditResultEnabled(!saving);
  setSaveEnabled(!saving);
}

void FitOutputOptionsPresenter::setPlotEnabled(bool enable) {
  m_view->setPlotEnabled(enable && isSelectedGroupPlottable());
}

void FitOutputOptionsPresenter::setEditResultEnabled(bool enable) { m_view->setEditResultEnabled(enable); }

void FitOutputOptionsPresenter::setSaveEnabled(bool enable) { m_view->setSaveEnabled(enable); }

void FitOutputOptionsPresenter::clearSpectraToPlot() { m_model->clearSpectraToPlot(); }

std::vector<SpectrumToPlot> FitOutputOptionsPresenter::getSpectraToPlot() const { return m_model->getSpectraToPlot(); }

void FitOutputOptionsPresenter::setEditResultVisible(bool visible) { m_view->setEditResultVisible(visible); }

void FitOutputOptionsPresenter::handleReplaceSingleFitResult(std::string const &inputName,
                                                             std::string const &singleBinName,
                                                             std::string const &outputName) {
  setEditingResult(true);
  replaceSingleFitResult(inputName, singleBinName, outputName);
  setEditingResult(false);
}

void FitOutputOptionsPresenter::replaceSingleFitResult(std::string const &inputName, std::string const &singleBinName,
                                                       std::string const &outputName) {
  try {
    m_model->replaceFitResult(inputName, singleBinName, outputName);
  } catch (std::exception const &ex) {
    m_view->displayWarning(ex.what());
  }
}

void FitOutputOptionsPresenter::setEditingResult(bool editing) {
  setPlotEnabled(!editing);
  setEditResultEnabled(!editing);
  setSaveEnabled(!editing);
}

void FitOutputOptionsPresenter::displayWarning(std::string const &message) { m_view->displayWarning(message); }

} // namespace MantidQt::CustomInterfaces::Inelastic
