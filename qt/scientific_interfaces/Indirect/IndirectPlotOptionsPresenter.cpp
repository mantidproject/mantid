// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectPlotOptionsPresenter.h"

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
      m_view(view), m_model(std::make_unique<IndirectPlotOptionsModel>()),
      m_parentTab(parent) {
  setupPresenter();
  m_view->setPlotType(plotType);
  m_view->setIndices(QString::fromStdString(fixedIndices));
  m_model->setFixedIndices(fixedIndices);
  setOptionsEnabled(false);
}

IndirectPlotOptionsPresenter::~IndirectPlotOptionsPresenter() {
  watchADS(false);
}

void IndirectPlotOptionsPresenter::setupPresenter() {
  watchADS(true);

  connect(m_view.get(), SIGNAL(selectedWorkspaceChanged(std::string const &)),
          this, SLOT(workspaceChanged(std::string const &)));
  connect(m_view.get(), SIGNAL(selectedIndicesChanged(std::string const &)),
          this, SLOT(indicesChanged(std::string const &)));

  connect(m_view.get(), SIGNAL(plotSpectraClicked()), this,
          SLOT(plotSpectra()));
  connect(m_view.get(), SIGNAL(plotBinsClicked()), this, SLOT(plotBins()));
  connect(m_view.get(), SIGNAL(plotContourClicked()), this,
          SLOT(plotContour()));
  connect(m_view.get(), SIGNAL(plotTiledClicked()), this, SLOT(plotTiled()));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  connect(&m_pythonRunner, SIGNAL(runAsPythonScript(QString const &, bool)),
          m_parentTab, SIGNAL(runAsPythonScript(QString const &, bool)));
#endif

  m_view->setIndicesRegex(QString::fromStdString(Regexes::WORKSPACE_INDICES));
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

void IndirectPlotOptionsPresenter::setOptionsEnabled(bool enable) {
  m_view->setWorkspaceComboBoxEnabled(enable);
  m_view->setIndicesLineEditEnabled(!m_model->indicesFixed() ? enable : false);
  m_view->setPlotButtonEnabled(enable);
}

void IndirectPlotOptionsPresenter::onWorkspaceRemoved(
    WorkspacePreDeleteNotification_ptr nf) {
  // Ignore non matrix workspaces
  if (auto const removedWorkspace =
          boost::dynamic_pointer_cast<MatrixWorkspace>(nf->object())) {
    auto const removedName = removedWorkspace->getName();
    if (removedName == m_view->selectedWorkspace().toStdString()) {
      m_model->removeWorkspace();
      m_view->removeWorkspace(QString::fromStdString(removedName));
    }
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

void IndirectPlotOptionsPresenter::removeWorkspace() {
  m_model->removeWorkspace();
  m_view->removeWorkspaces();
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
  indicesChanged(m_view->selectedIndices().toStdString());
}

void IndirectPlotOptionsPresenter::indicesChanged(std::string const &indices) {
  auto const formattedIndices = m_model->formatIndices(indices);
  m_view->setIndices(QString::fromStdString(formattedIndices));
  m_view->setIndicesErrorLabelVisible(!m_model->setIndices(formattedIndices));

  if (!formattedIndices.empty())
    m_view->addIndicesSuggestion(QString::fromStdString(formattedIndices));
}

void IndirectPlotOptionsPresenter::plotSpectra() {
  setOptionsEnabled(false);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  runPythonCode(m_model->getPlotSpectraString(m_parentTab->errorBars()));
#else
  m_model->plotSpectra(m_parentTab->errorBars());
#endif
  setOptionsEnabled(true);
}

void IndirectPlotOptionsPresenter::plotBins() {
  auto const indicesString = m_view->selectedIndices().toStdString();
  if (m_model->validateIndices(indicesString, MantidAxis::Bin)) {
    setOptionsEnabled(false);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    runPythonCode(
        m_model->getPlotBinsString(indicesString, m_parentTab->errorBars()));
#else
    m_model->plotBins(m_parentTab->errorBars());
#endif
    setOptionsEnabled(true);
  } else {
    m_view->displayWarning(
        "Failed to plot bins: Invalid bin indices provided.");
  }
}

void IndirectPlotOptionsPresenter::plotContour() {
  setOptionsEnabled(false);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  runPythonCode(m_model->getPlotContourString());
#else
  m_model->plotContour();
#endif
  setOptionsEnabled(true);
}

void IndirectPlotOptionsPresenter::plotTiled() {
  setOptionsEnabled(false);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  runPythonCode(m_model->getPlotTiledString());
#else
  m_model->plotTiled();
#endif
  setOptionsEnabled(true);
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
void IndirectPlotOptionsPresenter::runPythonCode(
    boost::optional<std::string> const &plotString) {
  if (plotString)
    m_pythonRunner.runPythonCode(QString::fromStdString(plotString.get()));
}
#endif

} // namespace CustomInterfaces
} // namespace MantidQt
