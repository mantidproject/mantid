// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectPlotOptionsView.h"
#include "IndirectPlotOptionsPresenter.h"

#include "MantidQtIcons/Icon.h"

#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSignalBlocker>

#include <stdexcept>

namespace {

auto constexpr SETTINGS_GROUP = "Indices suggestions";
auto constexpr SETTING_NAME = "Suggestions";
auto constexpr NUMBER_OF_SUGGESTIONS = 5;

// make sure the unit id is a valid unit factory id
// (https://docs.mantidproject.org/nightly/concepts/UnitFactory.html#id2)
const std::map<std::string, std::string> displayStrToUnitId = {
    {"D-Spacing", "dSpacing"},
    {"Q-Squared", "QSquared"},
};

void saveIndicesSuggestions(QStringList const &suggestions) {
  QSettings settings;
  settings.beginGroup(SETTINGS_GROUP);
  settings.setValue(SETTING_NAME, suggestions);
  settings.endGroup();
}

QStringList indicesSuggestions() {
  QSettings settings;
  settings.beginGroup(SETTINGS_GROUP);
  auto const suggestions = settings.value(SETTING_NAME).toStringList();
  settings.endGroup();

  return suggestions;
}

QString getAction(std::map<std::string, std::string> const &actions, std::string const &key) {
  auto const iter = actions.find(key);
  if (iter != actions.end())
    return QString::fromStdString(iter->second);
  return "";
}

QIcon plotCurveIcon() { return MantidQt::Icons::getIcon("mdi.chart-line"); }

QIcon showSliceViewerIcon() { return MantidQt::Icons::getIcon("mdi.chart-scatterplot-hexbin"); }

QIcon plotTiledIcon() { return MantidQt::Icons::getIcon("mdi.chart-line-stacked"); }

} // namespace

namespace MantidQt::CustomInterfaces {

IndirectPlotOptionsView::IndirectPlotOptionsView(QWidget *parent)
    : API::MantidWidget(parent), m_suggestionsModel(std::make_unique<QStringListModel>(indicesSuggestions())),
      m_completer(std::make_unique<QCompleter>(m_suggestionsModel.get(), this)),
      m_plotOptions(new Ui::IndirectPlotOptions), m_presenter() {
  m_plotOptions->setupUi(this);
  setupView();
}

void IndirectPlotOptionsView::setupView() {
  connect(m_plotOptions->cbWorkspace, SIGNAL(currentTextChanged(QString const &)), this,
          SLOT(notifySelectedWorkspaceChanged(QString const &)));

  connect(m_plotOptions->cbPlotUnit, SIGNAL(currentTextChanged(QString const &)), this,
          SLOT(notifySelectedUnitChanged(QString const &)));

  connect(m_plotOptions->leIndices, SIGNAL(editingFinished()), this, SLOT(notifySelectedIndicesChanged()));
  connect(m_plotOptions->leIndices, SIGNAL(textEdited(QString const &)), this,
          SLOT(notifySelectedIndicesChanged(QString const &)));

  connect(m_plotOptions->pbPlotSpectra, SIGNAL(clicked()), this, SLOT(notifyPlotSpectraClicked()));

  setIndicesErrorLabelVisible(false);

  // Setup the spectra auto-completer
  m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  m_completer->setMaxVisibleItems(NUMBER_OF_SUGGESTIONS);
  m_plotOptions->leIndices->setCompleter(m_completer.get());
}

void IndirectPlotOptionsView::subscribePresenter(IIndirectPlotOptionsPresenter *presenter) { m_presenter = presenter; }

void IndirectPlotOptionsView::notifySelectedWorkspaceChanged(QString const &workspaceName) {
  m_presenter->handleWorkspaceChanged(workspaceName.toStdString());
}

void IndirectPlotOptionsView::notifySelectedUnitChanged(QString const &unit) {
  if (!unit.isEmpty()) {
    m_presenter->handleSelectedUnitChanged(displayStrToUnitId.at(unit.toStdString()));
  }
}

void IndirectPlotOptionsView::notifySelectedIndicesChanged() {
  m_presenter->handleSelectedIndicesChanged(selectedIndices().toStdString());
}

void IndirectPlotOptionsView::notifySelectedIndicesChanged(QString const &spectra) {
  if (spectra.isEmpty()) {
    m_presenter->handleSelectedIndicesChanged(spectra.toStdString());
  }
}

void IndirectPlotOptionsView::notifyPlotSpectraClicked() {
  notifySelectedIndicesChanged();
  m_presenter->handlePlotSpectraClicked();
}

void IndirectPlotOptionsView::notifyPlotBinsClicked() {
  notifySelectedIndicesChanged();
  m_presenter->handlePlotBinsClicked();
}

void IndirectPlotOptionsView::notifyShowSliceViewerClicked() {
  notifySelectedIndicesChanged();
  m_presenter->handleShowSliceViewerClicked();
}

void IndirectPlotOptionsView::notifyPlotTiledClicked() {
  notifySelectedIndicesChanged();
  m_presenter->handlePlotTiledClicked();
}

void IndirectPlotOptionsView::setPlotType(PlotWidget const &plotType,
                                          std::map<std::string, std::string> const &availableActions) {
  auto plotMenu = new QMenu;

  auto plotSpectraAction = new QAction(getAction(availableActions, "Plot Spectra"), this);
  plotSpectraAction->setIcon(plotCurveIcon());
  auto plotBinAction = new QAction(getAction(availableActions, "Plot Bins"), this);
  plotBinAction->setIcon(plotCurveIcon());
  auto showSliceViewerAction = new QAction(getAction(availableActions, "Open Slice Viewer"), this);
  showSliceViewerAction->setIcon(showSliceViewerIcon());
  auto plotTiledAction = new QAction(getAction(availableActions, "Plot Tiled"), this);
  plotTiledAction->setIcon(plotTiledIcon());

  connect(plotSpectraAction, SIGNAL(triggered()), this, SLOT(notifyPlotSpectraClicked()));
  connect(plotBinAction, SIGNAL(triggered()), this, SLOT(notifyPlotBinsClicked()));
  connect(showSliceViewerAction, SIGNAL(triggered()), this, SLOT(notifyShowSliceViewerClicked()));
  connect(plotTiledAction, SIGNAL(triggered()), this, SLOT(notifyPlotTiledClicked()));

  m_plotOptions->tbPlot->setVisible(true);
  m_plotOptions->pbPlotSpectra->setVisible(true);
  m_plotOptions->pbPlotSpectra->setText(getAction(availableActions, "Plot Spectra"));
  m_plotOptions->cbPlotUnit->setVisible(false);

  switch (plotType) {
  case PlotWidget::Spectra:
    m_plotOptions->tbPlot->setVisible(false);
    break;
  case PlotWidget::SpectraBin:
    m_plotOptions->pbPlotSpectra->setVisible(false);
    plotMenu->addAction(plotSpectraAction);
    plotMenu->addAction(plotBinAction);
    break;
  case PlotWidget::SpectraSlice:
    m_plotOptions->pbPlotSpectra->setVisible(false);
    plotMenu->addAction(plotSpectraAction);
    plotMenu->addAction(showSliceViewerAction);
    break;
  case PlotWidget::SpectraTiled:
    m_plotOptions->pbPlotSpectra->setVisible(false);
    plotMenu->addAction(plotSpectraAction);
    plotMenu->addAction(plotTiledAction);
    break;
  case PlotWidget::SpectraUnit:
    m_plotOptions->tbPlot->setVisible(false);
    m_plotOptions->cbPlotUnit->setVisible(true);
    break;
  case PlotWidget::SpectraSliceUnit:
    m_plotOptions->pbPlotSpectra->setVisible(false);
    m_plotOptions->cbPlotUnit->setVisible(true);
    plotMenu->addAction(plotSpectraAction);
    plotMenu->addAction(showSliceViewerAction);
    break;
  default:
    std::runtime_error("Plot option not found. Plot types are Spectra, "
                       "SpectraSliced or SpectraTiled.");
  }
  m_plotOptions->tbPlot->setMenu(plotMenu);
  m_plotOptions->tbPlot->setDefaultAction(plotSpectraAction);

  m_plotOptions->cbPlotUnit->clear();
  for (const auto &item : displayStrToUnitId) {
    m_plotOptions->cbPlotUnit->addItem(QString::fromStdString(item.first));
  }
}

void IndirectPlotOptionsView::setWorkspaceComboBoxEnabled(bool enable) {
  QSignalBlocker blocker(m_plotOptions->cbWorkspace);
  m_plotOptions->cbWorkspace->setEnabled(enable);
}

void IndirectPlotOptionsView::setUnitComboBoxEnabled(bool enable) {
  QSignalBlocker blocker(m_plotOptions->cbPlotUnit);
  m_plotOptions->cbPlotUnit->setEnabled(enable);
}

void IndirectPlotOptionsView::setIndicesLineEditEnabled(bool enable) {
  QSignalBlocker blocker(m_plotOptions->leIndices);
  m_plotOptions->leIndices->setEnabled(enable);
}

void IndirectPlotOptionsView::setPlotButtonEnabled(bool enable) {
  m_plotOptions->pbPlotSpectra->setEnabled(enable);
  m_plotOptions->tbPlot->setEnabled(enable);
}

void IndirectPlotOptionsView::setPlotButtonText(QString const &text) {
  m_plotOptions->pbPlotSpectra->setText(text);
  m_plotOptions->tbPlot->setText(text);
}

void IndirectPlotOptionsView::setIndicesRegex(QString const &regex) {
  m_plotOptions->leIndices->setValidator(createValidator(regex));
}

QValidator *IndirectPlotOptionsView::createValidator(QString const &regex) {
  return new QRegExpValidator(QRegExp(regex), this);
}

QString IndirectPlotOptionsView::selectedWorkspace() const { return m_plotOptions->cbWorkspace->currentText(); }

void IndirectPlotOptionsView::setWorkspaces(std::vector<std::string> const &workspaces) {
  clearWorkspaces();
  for (auto const &name : workspaces)
    m_plotOptions->cbWorkspace->addItem(QString::fromStdString(name));
}

int IndirectPlotOptionsView::numberOfWorkspaces() const { return m_plotOptions->cbWorkspace->count(); }

void IndirectPlotOptionsView::clearWorkspaces() { m_plotOptions->cbWorkspace->clear(); }

void IndirectPlotOptionsView::removeWorkspace(QString const &workspaceName) {
  auto const index = m_plotOptions->cbWorkspace->findText(workspaceName);
  if (index != -1)
    m_plotOptions->cbWorkspace->removeItem(index);
}

QString IndirectPlotOptionsView::selectedIndices() const { return m_plotOptions->leIndices->text(); }

void IndirectPlotOptionsView::setIndices(QString const &indices) {
  QSignalBlocker blocker(m_plotOptions->leIndices);
  m_plotOptions->leIndices->setText(indices);
}

void IndirectPlotOptionsView::setIndicesErrorLabelVisible(bool visible) {
  m_plotOptions->lbIndicesError->setText(visible ? "*" : "");
  m_plotOptions->lbIndicesError->setVisible(visible);
}

void IndirectPlotOptionsView::addIndicesSuggestion(QString const &indices) {
  auto suggestions = m_suggestionsModel->stringList();
  if (!suggestions.contains(indices)) {
    if (suggestions.size() >= NUMBER_OF_SUGGESTIONS)
      suggestions.removeLast();
    suggestions.insert(suggestions.begin(), indices);
    m_suggestionsModel->setStringList(suggestions);

    saveIndicesSuggestions(suggestions);
  }
}

void IndirectPlotOptionsView::displayWarning(QString const &message) {
  QMessageBox::warning(parentWidget(), "Mantid - Warning", message);
}

} // namespace MantidQt::CustomInterfaces
