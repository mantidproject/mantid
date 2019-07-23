// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectPlotOptionsView.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QMenu>
#include <QMessageBox>
#include <QSettings>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtIcons/Icon.h"
#endif

namespace {

auto constexpr SETTINGS_GROUP = "Indices suggestions";
auto constexpr SETTING_NAME = "Suggestions";
auto constexpr NUMBER_OF_SUGGESTIONS = 5;

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

QIcon plotCurveIcon() {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  return QIcon(":/curves.png");
#else
  return MantidQt::Icons::getIcon("mdi.chart-line");
#endif
}

QIcon plotContourIcon() {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  return QIcon(":/contour_map.png");
#else
  return MantidQt::Icons::getIcon("mdi.chart-scatterplot-hexbin");
#endif
}

QIcon plotTiledIcon() {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  return QIcon(":/arrangeLayers.png");
#else
  return MantidQt::Icons::getIcon("mdi.chart-line-stacked");
#endif
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {

IndirectPlotOptionsView::IndirectPlotOptionsView(QWidget *parent)
    : API::MantidWidget(parent),
      m_suggestionsModel(
          std::make_unique<QStringListModel>(indicesSuggestions())),
      m_completer(std::make_unique<QCompleter>(m_suggestionsModel.get(), this)),
      m_plotOptions(new Ui::IndirectPlotOptions) {
  m_plotOptions->setupUi(this);
  setupView();
}

void IndirectPlotOptionsView::setupView() {
  connect(m_plotOptions->cbWorkspace,
          SIGNAL(currentIndexChanged(QString const &)), this,
          SLOT(emitSelectedWorkspaceChanged(QString const &)));

  connect(m_plotOptions->leIndices, SIGNAL(editingFinished()), this,
          SLOT(emitSelectedIndicesChanged()));
  connect(m_plotOptions->leIndices, SIGNAL(textEdited(QString const &)), this,
          SLOT(emitSelectedIndicesChanged(QString const &)));

  setIndicesErrorLabelVisible(false);

  // Setup the spectra auto-completer
  m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  m_completer->setMaxVisibleItems(NUMBER_OF_SUGGESTIONS);
  m_plotOptions->leIndices->setCompleter(m_completer.get());
}

void IndirectPlotOptionsView::emitSelectedWorkspaceChanged(
    QString const &workspaceName) {
  emit selectedWorkspaceChanged(workspaceName.toStdString());
}

void IndirectPlotOptionsView::emitSelectedIndicesChanged() {
  emit selectedIndicesChanged(selectedIndices().toStdString());
}

void IndirectPlotOptionsView::emitSelectedIndicesChanged(
    QString const &spectra) {
  if (spectra.isEmpty())
    emit selectedIndicesChanged(spectra.toStdString());
}

void IndirectPlotOptionsView::emitPlotSpectraClicked() {
  emit plotSpectraClicked();
}

void IndirectPlotOptionsView::emitPlotBinsClicked() { emit plotBinsClicked(); }

void IndirectPlotOptionsView::emitPlotContourClicked() {
  emit plotContourClicked();
}

void IndirectPlotOptionsView::emitPlotTiledClicked() {
  emit plotTiledClicked();
}

void IndirectPlotOptionsView::setPlotType(PlotWidget const &plotType) {
  auto plotMenu = new QMenu;

  auto plotSpectraAction = new QAction("Plot Spectra", this);
  plotSpectraAction->setIcon(plotCurveIcon());
  auto plotBinAction = new QAction("Plot Bins", this);
  plotBinAction->setIcon(plotCurveIcon());
  auto plotContourAction = new QAction("Plot Contour", this);
  plotContourAction->setIcon(plotContourIcon());
  auto plotTiledAction = new QAction("Plot Tiled", this);
  plotTiledAction->setIcon(plotTiledIcon());

  connect(plotSpectraAction, SIGNAL(triggered()), this,
          SLOT(emitPlotSpectraClicked()));
  connect(plotBinAction, SIGNAL(triggered()), this,
          SLOT(emitPlotBinsClicked()));
  connect(plotContourAction, SIGNAL(triggered()), this,
          SLOT(emitPlotContourClicked()));
  connect(plotTiledAction, SIGNAL(triggered()), this,
          SLOT(emitPlotTiledClicked()));

  switch (plotType) {
  case PlotWidget::Spectra:
    plotMenu->addAction(plotSpectraAction);
    break;
  case PlotWidget::SpectraBin:
    plotMenu->addAction(plotSpectraAction);
    plotMenu->addAction(plotBinAction);
    break;
  case PlotWidget::SpectraContour:
    plotMenu->addAction(plotSpectraAction);
    plotMenu->addAction(plotContourAction);
    break;
  case PlotWidget::SpectraTiled:
    plotMenu->addAction(plotSpectraAction);
    plotMenu->addAction(plotTiledAction);
    break;
  default:
    std::runtime_error("Plot option not found. Plot types are Spectra, "
                       "SpectraContour or SpectraTiled.");
  }
  m_plotOptions->tbPlot->setMenu(plotMenu);
  m_plotOptions->tbPlot->setDefaultAction(plotSpectraAction);
}

void IndirectPlotOptionsView::setWorkspaceComboBoxEnabled(bool enable) {
  API::SignalBlocker blocker(m_plotOptions->cbWorkspace);
  m_plotOptions->cbWorkspace->setEnabled(enable);
}

void IndirectPlotOptionsView::setIndicesLineEditEnabled(bool enable) {
  API::SignalBlocker blocker(m_plotOptions->leIndices);
  m_plotOptions->leIndices->setEnabled(enable);
}

void IndirectPlotOptionsView::setPlotButtonEnabled(bool enable) {
  m_plotOptions->tbPlot->setEnabled(enable);
}

void IndirectPlotOptionsView::setPlotButtonText(QString const &text) {
  m_plotOptions->tbPlot->setText(text);
}

void IndirectPlotOptionsView::setIndicesRegex(QString const &regex) {
  m_plotOptions->leIndices->setValidator(createValidator(regex));
}

QValidator *IndirectPlotOptionsView::createValidator(QString const &regex) {
  return new QRegExpValidator(QRegExp(regex), this);
}

QString IndirectPlotOptionsView::selectedWorkspace() const {
  return m_plotOptions->cbWorkspace->currentText();
}

void IndirectPlotOptionsView::setWorkspaces(
    std::vector<std::string> const &workspaces) {
  clearWorkspaces();
  for (auto const &name : workspaces)
    m_plotOptions->cbWorkspace->addItem(QString::fromStdString(name));
}

int IndirectPlotOptionsView::numberOfWorkspaces() const {
  return m_plotOptions->cbWorkspace->count();
}

void IndirectPlotOptionsView::clearWorkspaces() {
  m_plotOptions->cbWorkspace->clear();
}

void IndirectPlotOptionsView::removeWorkspace(QString const &workspaceName) {
  auto const index = m_plotOptions->cbWorkspace->findText(workspaceName);
  if (index != -1)
    m_plotOptions->cbWorkspace->removeItem(index);
}

QString IndirectPlotOptionsView::selectedIndices() const {
  return m_plotOptions->leIndices->text();
}

void IndirectPlotOptionsView::setIndices(QString const &indices) {
  API::SignalBlocker blocker(m_plotOptions->leIndices);
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

} // namespace CustomInterfaces
} // namespace MantidQt
