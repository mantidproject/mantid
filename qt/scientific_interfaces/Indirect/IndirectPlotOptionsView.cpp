// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectPlotOptionsView.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QSettings>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtIcons/Icon.h"
#endif

namespace {

auto constexpr SETTINGS_GROUP = "Spectra suggestions";
auto constexpr SETTING_NAME = "Suggestions";
auto constexpr NUMBER_OF_SUGGESTIONS = 5;

void saveSpectraSuggestions(QStringList const &suggestions) {
  QSettings settings;
  settings.beginGroup(SETTINGS_GROUP);
  settings.setValue(SETTING_NAME, suggestions);
  settings.endGroup();
}

QStringList spectraSuggestions() {
  QSettings settings;
  settings.beginGroup(SETTINGS_GROUP);
  auto const suggestions = settings.value(SETTING_NAME).toStringList();
  settings.endGroup();

  return suggestions;
}

QIcon plotSpectraIcon() {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  return QIcon(":/plot_double_y.png");
#else
  return MantidQt::Icons::getIcon("mdi.chart-line");
#endif
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectPlotOptionsView::IndirectPlotOptionsView(QWidget *parent,
                                                 PlotWidget const &plotType)
    : API::MantidWidget(parent),
      m_suggestionsModel(
          std::make_unique<QStringListModel>(spectraSuggestions())),
      m_completer(std::make_unique<QCompleter>(m_suggestionsModel.get(), this)),
      m_plotOptions(new Ui::IndirectPlotOptions) {
  m_plotOptions->setupUi(this);
  m_plotOptions->pbPlotSpectra->setIcon(plotSpectraIcon());
  setupView();
}

void IndirectPlotOptionsView::setupView() {
  connect(m_plotOptions->leSpectra, SIGNAL(editingFinished()), this,
          SLOT(emitSelectedSpectraChanged()));
  connect(m_plotOptions->leSpectra, SIGNAL(textEdited(QString const &)), this,
          SLOT(emitSelectedSpectraChanged(QString const &)));

  connect(m_plotOptions->pbPlotSpectra, SIGNAL(clicked()), this,
          SLOT(emitPlotSpectraClicked()));
  connect(m_plotOptions->pbPlotContour, SIGNAL(clicked()), this,
          SLOT(emitPlotContourClicked()));
  connect(m_plotOptions->pbPlotTiled, SIGNAL(clicked()), this,
          SLOT(emitPlotTiledClicked()));

  setSpectraErrorLabelVisible(false);

  // Setup the spectra auto-completer
  m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
  m_completer->setMaxVisibleItems(NUMBER_OF_SUGGESTIONS);
  m_plotOptions->leSpectra->setCompleter(m_completer.get());
}

void IndirectPlotOptionsView::emitSelectedSpectraChanged() {
  emit selectedSpectraChanged(selectedSpectra().toStdString());
}

void IndirectPlotOptionsView::emitSelectedSpectraChanged(
    QString const &spectra) {
  if (spectra.isEmpty())
    emit selectedSpectraChanged(spectra.toStdString());
}

void IndirectPlotOptionsView::emitPlotSpectraClicked() {
  emit plotSpectraClicked();
}

void IndirectPlotOptionsView::emitPlotContourClicked() {
  emit plotContourClicked();
}

void IndirectPlotOptionsView::emitPlotTiledClicked() {
  emit plotTiledClicked();
}

void IndirectPlotOptionsView::setPlotType(PlotWidget const &plotType) {
  switch (plotType) {
  case PlotWidget::Spectra:
    m_plotOptions->pbPlotContour->setVisible(false);
    m_plotOptions->pbPlotTiled->setVisible(false);
    break;
  case PlotWidget::SpectraContour:
    m_plotOptions->pbPlotTiled->setVisible(false);
    break;
  case PlotWidget::SpectraTiled:
    m_plotOptions->pbPlotContour->setVisible(false);
    break;
  default:
    std::runtime_error("Plot option not found. Plot types are Spectra, "
                       "SpectraContour or SpectraTiled.");
  }
}

void IndirectPlotOptionsView::setOptionsEnabled(Plotting const &type,
                                                bool enable) {
  m_plotOptions->leSpectra->setEnabled(enable);
  m_plotOptions->pbPlotSpectra->setEnabled(enable);
  m_plotOptions->pbPlotContour->setEnabled(enable);
  m_plotOptions->pbPlotTiled->setEnabled(enable);
  if (type != Plotting::None)
    setButtonText(type, enable);
}

void IndirectPlotOptionsView::setButtonText(Plotting const &type,
                                            bool enabled) {
  switch (type) {
  case Plotting::Spectrum:
    // m_plotOptions->pbPlotSpectra->setText(enabled ? "Plot Spectra"
    //                                              : "Plotting...");
    break;
  case Plotting::Contour:
    m_plotOptions->pbPlotContour->setText(enabled ? "Plot Contour"
                                                  : "Plotting...");
    break;
  case Plotting::Tiled:
    m_plotOptions->pbPlotTiled->setText(enabled ? "Plot Tiled" : "Plotting...");
    break;
  }
}

void IndirectPlotOptionsView::setSpectraRegex(QString const &regex) {
  m_plotOptions->leSpectra->setValidator(createValidator(regex));
}

QValidator *IndirectPlotOptionsView::createValidator(QString const &regex) {
  return new QRegExpValidator(QRegExp(regex), this);
}

QString IndirectPlotOptionsView::selectedSpectra() const {
  return m_plotOptions->leSpectra->text();
}

void IndirectPlotOptionsView::setSpectra(QString const &spectra) {
  API::SignalBlocker blocker(m_plotOptions->leSpectra);
  m_plotOptions->leSpectra->setText(spectra);
}

void IndirectPlotOptionsView::setSpectraErrorLabelVisible(bool visible) {
  m_plotOptions->lbSpectraError->setText(visible ? "*" : "");
  m_plotOptions->lbSpectraError->setVisible(visible);
}

void IndirectPlotOptionsView::addSpectraSuggestion(QString const &spectra) {
  auto suggestions = m_suggestionsModel->stringList();
  if (!suggestions.contains(spectra)) {
    if (suggestions.size() >= NUMBER_OF_SUGGESTIONS)
      suggestions.removeLast();
    suggestions.insert(suggestions.begin(), spectra);
    m_suggestionsModel->setStringList(suggestions);

    saveSpectraSuggestions(suggestions);
  }
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
