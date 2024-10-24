// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsPresenter.h"

#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;

namespace {

std::string OR(std::string const &lhs, std::string const &rhs) { return "(" + lhs + "|" + rhs + ")"; }

std::string NATURAL_NUMBER(std::size_t const &digits) {
  return OR("0", "[1-9][0-9]{," + std::to_string(digits - 1) + "}");
}

namespace Regexes {

std::string const SPACE = "[ ]*";
std::string const COMMA = SPACE + "," + SPACE;
std::string const MINUS = "\\-";

std::string const NUMBER = NATURAL_NUMBER(4);
std::string const NATURAL_RANGE = "(" + NUMBER + MINUS + NUMBER + ")";
std::string const NATURAL_OR_RANGE = OR(NATURAL_RANGE, NUMBER);
std::string const WORKSPACE_INDICES = "(" + NATURAL_OR_RANGE + "(" + COMMA + NATURAL_OR_RANGE + ")*)";

} // namespace Regexes
} // namespace

namespace MantidQt::CustomInterfaces {
/// Used by the unit tests so that m_plotter can be mocked
OutputPlotOptionsPresenter::OutputPlotOptionsPresenter(IOutputPlotOptionsView *view,
                                                       std::unique_ptr<IOutputPlotOptionsModel> model,
                                                       PlotWidget const &plotType, std::string const &fixedIndices)
    : m_view(view), m_model(std::move(model)) {
  setupPresenter(plotType, fixedIndices);
}

void OutputPlotOptionsPresenter::setupPresenter(PlotWidget const &plotType, std::string const &fixedIndices) {
  watchADS(true);
  m_view->subscribePresenter(this);
  m_view->setIndicesRegex(QString::fromStdString(Regexes::WORKSPACE_INDICES));
  m_view->setPlotType(plotType, m_model->availableActions());
  m_view->setIndices(QString::fromStdString(fixedIndices));
  m_model->setFixedIndices(fixedIndices);

  m_plotType = plotType;
  setOptionsEnabled(false);
}

void OutputPlotOptionsPresenter::watchADS(bool on) {
  this->observeReplace(on);
  this->observeDelete(on);
}

void OutputPlotOptionsPresenter::setPlotType(PlotWidget const &plotType) {
  m_plotType = plotType;
  m_view->setPlotType(plotType, m_model->availableActions());
}

void OutputPlotOptionsPresenter::setPlotting(bool plotting) {
  m_view->setPlotButtonText(plotting ? "Plotting..."
                                     : QString::fromStdString(m_model->availableActions()["Plot Spectra"]));
  setOptionsEnabled(!plotting);
}

void OutputPlotOptionsPresenter::setOptionsEnabled(bool enable) {
  m_view->setWorkspaceComboBoxEnabled(m_view->numberOfWorkspaces() > 1 ? enable : false);
  m_view->setIndicesLineEditEnabled(!m_model->indicesFixed() ? enable : false);
  m_view->setPlotButtonEnabled(enable);
  m_view->setUnitComboBoxEnabled(enable);
}

void OutputPlotOptionsPresenter::deleteHandle(const std::string &wsName, const Workspace_sptr &workspace) {
  (void)workspace;
  if (wsName == m_view->selectedWorkspace().toStdString())
    m_model->removeWorkspace();
  m_view->removeWorkspace(QString::fromStdString(wsName));
}

void OutputPlotOptionsPresenter::replaceHandle(const std::string &wsName, const Workspace_sptr &workspace) {
  (void)wsName;
  // Ignore non matrix workspaces
  if (auto const newWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(workspace)) {
    auto const &newName = newWorkspace->getName();
    if (newName == m_view->selectedWorkspace().toStdString())
      handleWorkspaceChanged(newName);
  }
}

void OutputPlotOptionsPresenter::setWorkspaces(std::vector<std::string> const &workspaces) {
  auto const workspaceNames = m_model->getAllWorkspaceNames(workspaces);
  m_view->setWorkspaces(workspaceNames);
  handleWorkspaceChanged(!workspaceNames.empty() ? workspaceNames.front() : "");
}

void OutputPlotOptionsPresenter::setWorkspace(std::string const &plotWorkspace) {
  bool success = m_model->setWorkspace(plotWorkspace);
  setOptionsEnabled(success);
  if (success && !m_model->indicesFixed())
    setIndices();
}

void OutputPlotOptionsPresenter::clearWorkspaces() {
  m_model->removeWorkspace();
  m_view->clearWorkspaces();
  setOptionsEnabled(false);
}

void OutputPlotOptionsPresenter::setUnit(std::string const &unit) {
  if (m_plotType == PlotWidget::SpectraUnit || m_plotType == PlotWidget::SpectraSliceSurfaceUnit) {
    m_model->setUnit(unit);
  }
}

void OutputPlotOptionsPresenter::setIndices() {
  auto const selectedIndices = m_view->selectedIndices().toStdString();
  if (auto const indices = m_model->indices())
    handleSelectedIndicesChanged(indices.value());
  else if (!selectedIndices.empty())
    handleSelectedIndicesChanged(selectedIndices);
  else
    handleSelectedIndicesChanged("0");
}

void OutputPlotOptionsPresenter::handleWorkspaceChanged(std::string const &workspaceName) {
  setWorkspace(workspaceName);
}

void OutputPlotOptionsPresenter::handleSelectedUnitChanged(std::string const &unit) { setUnit(unit); }

void OutputPlotOptionsPresenter::handleSelectedIndicesChanged(std::string const &indices) {
  auto const formattedIndices = m_model->formatIndices(indices);
  m_view->setIndices(QString::fromStdString(formattedIndices));
  m_view->setIndicesErrorLabelVisible(!m_model->setIndices(formattedIndices));

  if (!formattedIndices.empty())
    m_view->addIndicesSuggestion(QString::fromStdString(formattedIndices));
}

void OutputPlotOptionsPresenter::handlePlotSpectraClicked() {
  if (validateWorkspaceSize(MantidAxis::Spectrum)) {
    setPlotting(true);
    m_model->plotSpectra();
    setPlotting(false);
  }
}

void OutputPlotOptionsPresenter::handlePlotBinsClicked() {
  if (validateWorkspaceSize(MantidAxis::Bin)) {
    auto const indicesString = m_view->selectedIndices().toStdString();
    if (m_model->validateIndices(indicesString, MantidAxis::Bin)) {
      setPlotting(true);
      m_model->plotBins(indicesString);
      setPlotting(false);
    } else {
      m_view->displayWarning("Plot Bins failed: Invalid bin indices provided.");
    }
  }
}

void OutputPlotOptionsPresenter::handleShowSliceViewerClicked() {
  if (validateWorkspaceSize(MantidAxis::Both)) {
    setPlotting(true);
    m_model->showSliceViewer();
    setPlotting(false);
  }
}

void OutputPlotOptionsPresenter::handlePlot3DClicked() {
  if (validateWorkspaceSize(MantidAxis::Both)) {
    setPlotting(true);
    m_model->plot3DSurface();
    setPlotting(false);
  }
}

void OutputPlotOptionsPresenter::handlePlotTiledClicked() {
  if (validateWorkspaceSize(MantidAxis::Spectrum)) {
    setPlotting(true);
    m_model->plotTiled();
    setPlotting(false);
  }
}

bool OutputPlotOptionsPresenter::validateWorkspaceSize(MantidAxis const &axisType) {
  auto const errorMessage = m_model->singleDataPoint(axisType);
  if (errorMessage) {
    m_view->displayWarning(QString::fromStdString(errorMessage.value()));
    return false;
  }
  return true;
}

} // namespace MantidQt::CustomInterfaces
