// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectPlotOptionsPresenter.h"

#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;

namespace {

std::string OR(std::string const &lhs, std::string const &rhs) {
  return "(" + lhs + "|" + rhs + ")";
}

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
std::string const WORKSPACE_INDICES =
    "(" + NATURAL_OR_RANGE + "(" + COMMA + NATURAL_OR_RANGE + ")*)";

} // namespace Regexes
} // namespace

namespace MantidQt {
namespace CustomInterfaces {

IndirectPlotOptionsPresenter::IndirectPlotOptionsPresenter(
    IndirectPlotOptionsView *view, IndirectTab *parent,
    PlotWidget const &plotType, std::string const &fixedIndices)
    : QObject(nullptr),
      m_wsRemovedObserver(*this,
                          &IndirectPlotOptionsPresenter::onWorkspaceRemoved),
      m_wsReplacedObserver(*this,
                           &IndirectPlotOptionsPresenter::onWorkspaceReplaced),
      m_view(view),
      m_model(std::make_unique<IndirectPlotOptionsModel>(parent)) {
  setupPresenter(plotType, fixedIndices);
}

/// Used by the unit tests so that m_plotter can be mocked
IndirectPlotOptionsPresenter::IndirectPlotOptionsPresenter(
    IndirectPlotOptionsView *view, IndirectPlotOptionsModel *model,
    PlotWidget const &plotType, std::string const &fixedIndices)
    : QObject(nullptr),
      m_wsRemovedObserver(*this,
                          &IndirectPlotOptionsPresenter::onWorkspaceRemoved),
      m_wsReplacedObserver(*this,
                           &IndirectPlotOptionsPresenter::onWorkspaceReplaced),
      m_view(view), m_model(std::move(model)) {
  setupPresenter(plotType, fixedIndices);
}

IndirectPlotOptionsPresenter::~IndirectPlotOptionsPresenter() {
  watchADS(false);
}

void IndirectPlotOptionsPresenter::setupPresenter(
    PlotWidget const &plotType, std::string const &fixedIndices) {
  watchADS(true);

  connect(m_view, SIGNAL(selectedWorkspaceChanged(std::string const &)), this,
          SLOT(workspaceChanged(std::string const &)));
  connect(m_view, SIGNAL(selectedIndicesChanged(std::string const &)), this,
          SLOT(indicesChanged(std::string const &)));

  connect(m_view, SIGNAL(plotSpectraClicked()), this, SLOT(plotSpectra()));
  connect(m_view, SIGNAL(plotBinsClicked()), this, SLOT(plotBins()));
  connect(m_view, SIGNAL(plotContourClicked()), this, SLOT(plotContour()));
  connect(m_view, SIGNAL(plotTiledClicked()), this, SLOT(plotTiled()));

  m_view->setIndicesRegex(QString::fromStdString(Regexes::WORKSPACE_INDICES));
  m_view->setPlotType(plotType);
  m_view->setIndices(QString::fromStdString(fixedIndices));
  m_model->setFixedIndices(fixedIndices);

  setOptionsEnabled(false);
}

void IndirectPlotOptionsPresenter::watchADS(bool on) {
  auto &notificationCenter = AnalysisDataService::Instance().notificationCenter;
  if (on) {
    notificationCenter.addObserver(m_wsRemovedObserver);
    notificationCenter.addObserver(m_wsReplacedObserver);
  } else {
    notificationCenter.removeObserver(m_wsReplacedObserver);
    notificationCenter.removeObserver(m_wsRemovedObserver);
  }
}

void IndirectPlotOptionsPresenter::setPlotting(bool plotting) {
  m_view->setPlotButtonText(plotting ? "Plotting..." : "Plot Spectra");
  setOptionsEnabled(!plotting);
}

void IndirectPlotOptionsPresenter::setOptionsEnabled(bool enable) {
  m_view->setWorkspaceComboBoxEnabled(m_view->numberOfWorkspaces() > 1 ? enable
                                                                       : false);
  m_view->setIndicesLineEditEnabled(!m_model->indicesFixed() ? enable : false);
  m_view->setPlotButtonEnabled(enable);
}

void IndirectPlotOptionsPresenter::onWorkspaceRemoved(
    WorkspacePreDeleteNotification_ptr nf) {
  // Ignore non matrix workspaces
  if (auto const removedWorkspace =
          boost::dynamic_pointer_cast<MatrixWorkspace>(nf->object())) {
    auto const removedName = removedWorkspace->getName();
    if (removedName == m_view->selectedWorkspace().toStdString())
      m_model->removeWorkspace();
    m_view->removeWorkspace(QString::fromStdString(removedName));
  }
}

void IndirectPlotOptionsPresenter::onWorkspaceReplaced(
    WorkspaceBeforeReplaceNotification_ptr nf) {
  // Ignore non matrix workspaces
  if (auto const newWorkspace =
          boost::dynamic_pointer_cast<MatrixWorkspace>(nf->newObject())) {
    auto const newName = newWorkspace->getName();
    if (newName == m_view->selectedWorkspace().toStdString())
      workspaceChanged(newName);
  }
}

void IndirectPlotOptionsPresenter::setWorkspaces(
    std::vector<std::string> const &workspaces) {
  m_view->setWorkspaces(workspaces);
  workspaceChanged(workspaces.front());
}

void IndirectPlotOptionsPresenter::setWorkspace(
    std::string const &plotWorkspace) {
  bool success = m_model->setWorkspace(plotWorkspace);
  setOptionsEnabled(success);
  if (success && !m_model->indicesFixed())
    setIndices();
}

void IndirectPlotOptionsPresenter::clearWorkspaces() {
  m_model->removeWorkspace();
  m_view->clearWorkspaces();
  setOptionsEnabled(false);
}

void IndirectPlotOptionsPresenter::setIndices() {
  auto const selectedIndices = m_view->selectedIndices().toStdString();
  if (auto const indices = m_model->indices())
    indicesChanged(indices.get());
  else if (!selectedIndices.empty())
    indicesChanged(selectedIndices);
  else
    indicesChanged("0");
}

void IndirectPlotOptionsPresenter::workspaceChanged(
    std::string const &workspaceName) {
  setWorkspace(workspaceName);
}

void IndirectPlotOptionsPresenter::indicesChanged(std::string const &indices) {
  auto const formattedIndices = m_model->formatIndices(indices);
  m_view->setIndices(QString::fromStdString(formattedIndices));
  m_view->setIndicesErrorLabelVisible(!m_model->setIndices(formattedIndices));

  if (!formattedIndices.empty())
    m_view->addIndicesSuggestion(QString::fromStdString(formattedIndices));
}

void IndirectPlotOptionsPresenter::plotSpectra() {
  setPlotting(true);
  m_model->plotSpectra();
  setPlotting(false);
}

void IndirectPlotOptionsPresenter::plotBins() {
  auto const indicesString = m_view->selectedIndices().toStdString();
  if (m_model->validateIndices(indicesString, MantidAxis::Bin)) {
    setPlotting(true);
    m_model->plotBins();
    setPlotting(false);
  } else {
    m_view->displayWarning("Plot bins failed: Invalid bin indices provided.");
  }
}

void IndirectPlotOptionsPresenter::plotContour() {
  setPlotting(true);
  m_model->plotContour();
  setPlotting(false);
}

void IndirectPlotOptionsPresenter::plotTiled() {
  setPlotting(true);
  m_model->plotTiled();
  setPlotting(false);
}

} // namespace CustomInterfaces
} // namespace MantidQt
