// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPlotViewLegacy.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <boost/numeric/conversion/cast.hpp>

#include <QMessageBox>
#include <QTimer>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtIcons/Icon.h"

namespace {

QHash<QString, QVariant> tightLayoutKwargs() {
  QHash<QString, QVariant> kwargs;
  kwargs.insert("pad", 0);
  return kwargs;
}

} // namespace

#endif

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace MantidWidgets;

IndirectFitPlotViewLegacy::IndirectFitPlotViewLegacy(QWidget *parent)
    : IIndirectFitPlotViewLegacy(parent), m_plotForm(new Ui::IndirectFitPreviewPlot) {
  m_plotForm->setupUi(this);

  connect(m_plotForm->cbDataSelection, SIGNAL(currentIndexChanged(int)), this,
          SLOT(emitSelectedFitDataChanged(int)));
  connect(m_plotForm->spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitDelayedPlotSpectrumChanged()));

  connect(m_plotForm->cbPlotSpectrum,
          SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(emitPlotSpectrumChanged(const QString &)));
  connect(m_plotForm->ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(emitPlotGuessChanged(int)));
  connect(m_plotForm->pbPlotPreview, SIGNAL(clicked()), this,
          SIGNAL(plotCurrentPreview()));
  connect(m_plotForm->pbFitSingle, SIGNAL(clicked()), this,
          SIGNAL(fitSelectedSpectrum()));

  // Create a Splitter and place two plots within the splitter layout
  createSplitterWithPlots();

  // Avoids squished plots for >qt5
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  char const *const overrideLabel = "";
  m_topPlot->setOverrideAxisLabel(AxisID::XBottom, overrideLabel);
  m_bottomPlot->setOverrideAxisLabel(AxisID::YLeft, overrideLabel);
  m_plotForm->dwMiniPlots->setFeatures(QDockWidget::NoDockWidgetFeatures);
#endif

  m_plotForm->cbDataSelection->hide();
  addFitRangeSelector();
  addBackgroundRangeSelector();
  addHWHMRangeSelector();
}

void IndirectFitPlotViewLegacy::createSplitterWithPlots() {
  createSplitter();
  m_splitter->addWidget(createTopPlot());
  m_splitter->addWidget(createBottomPlot());

  m_plotForm->gridLayout->addWidget(m_splitter.get(), 0, 0, 1, 1);
}

void IndirectFitPlotViewLegacy::createSplitter() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  auto const dragIcon = Icons::getIcon("mdi.dots-horizontal");
  m_splitter = std::make_unique<Splitter>(dragIcon, m_plotForm->dwLayout);
#else
  m_splitter = std::make_unique<QSplitter>(m_plotForm->dwLayout);
#endif
  m_splitter->setOrientation(Qt::Vertical);
  m_splitter->setStyleSheet(
      "QSplitter::handle { background-color: transparent; }");
}

PreviewPlot *IndirectFitPlotViewLegacy::createTopPlot() {
  m_topPlot = std::make_unique<PreviewPlot>(m_splitter.get());
  return createPlot(m_topPlot.get(), QSize(0, 125), 0, 10);
}

PreviewPlot *IndirectFitPlotViewLegacy::createBottomPlot() {
  m_bottomPlot = std::make_unique<PreviewPlot>(m_splitter.get());
  return createPlot(m_bottomPlot.get(), QSize(0, 75), 0, 6);
}

PreviewPlot *
IndirectFitPlotViewLegacy::createPlot(PreviewPlot *plot, QSize const &minimumSize,
                                unsigned char horizontalStretch,
                                unsigned char verticalStretch) const {
  setPlotSizePolicy(plot, horizontalStretch, verticalStretch);

  plot->setMinimumSize(minimumSize);
  plot->setProperty("showLegend", QVariant(true));
  plot->setProperty("canvasColour", QVariant(QColor(255, 255, 255)));

  // Avoids squished plots for >qt5
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  plot->setTightLayout(tightLayoutKwargs());
#endif

  return plot;
}

void IndirectFitPlotViewLegacy::setPlotSizePolicy(
    PreviewPlot *plot, unsigned char horizontalStretch,
    unsigned char verticalStretch) const {
  QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  sizePolicy.setHorizontalStretch(horizontalStretch);
  sizePolicy.setVerticalStretch(verticalStretch);
  sizePolicy.setHeightForWidth(plot->sizePolicy().hasHeightForWidth());
  plot->setSizePolicy(sizePolicy);
}

IndirectFitPlotViewLegacy::~IndirectFitPlotViewLegacy() {
  m_topPlot.reset();
  m_bottomPlot.reset();
  m_splitter.reset();
}

void IndirectFitPlotViewLegacy::watchADS(bool watch) {
  m_topPlot->watchADS(watch);
  m_bottomPlot->watchADS(watch);
}

std::string IndirectFitPlotViewLegacy::getSpectrumText() const {
  return m_plotForm->cbPlotSpectrum->currentText().toStdString();
}

std::size_t IndirectFitPlotViewLegacy::getSelectedSpectrum() const {
  if (m_plotForm->swPlotSpectrum->currentIndex() == 0)
    return m_plotForm->spPlotSpectrum->value();
  else if (m_plotForm->cbPlotSpectrum->count() != 0)
    return std::stoull(getSpectrumText());
  return 0;
}

int IndirectFitPlotViewLegacy::getSelectedSpectrumIndex() const {
  if (m_plotForm->swPlotSpectrum->currentIndex() == 0)
    return m_plotForm->spPlotSpectrum->value() -
           m_plotForm->spPlotSpectrum->minimum();
  return m_plotForm->cbPlotSpectrum->currentIndex();
}

int IndirectFitPlotViewLegacy::getSelectedDataIndex() const {
  return m_plotForm->cbDataSelection->currentIndex();
}

std::size_t IndirectFitPlotViewLegacy::dataSelectionSize() const {
  return boost::numeric_cast<std::size_t>(m_plotForm->cbDataSelection->count());
}

bool IndirectFitPlotViewLegacy::isPlotGuessChecked() const {
  return m_plotForm->ckPlotGuess->isChecked();
}

void IndirectFitPlotViewLegacy::hideMultipleDataSelection() {
  m_plotForm->cbDataSelection->hide();
}

void IndirectFitPlotViewLegacy::showMultipleDataSelection() {
  m_plotForm->cbDataSelection->show();
}

void IndirectFitPlotViewLegacy::setAvailableSpectra(std::size_t minimum,
                                              std::size_t maximum) {
  m_plotForm->swPlotSpectrum->setCurrentIndex(0);
  m_plotForm->spPlotSpectrum->setMinimum(boost::numeric_cast<int>(minimum));
  m_plotForm->spPlotSpectrum->setMaximum(boost::numeric_cast<int>(maximum));
}

void IndirectFitPlotViewLegacy::setAvailableSpectra(
    const std::vector<std::size_t>::const_iterator &from,
    const std::vector<std::size_t>::const_iterator &to) {
  m_plotForm->swPlotSpectrum->setCurrentIndex(1);
  m_plotForm->cbPlotSpectrum->clear();

  for (auto spectrum = from; spectrum < to; ++spectrum)
    m_plotForm->cbPlotSpectrum->addItem(QString::number(*spectrum));
}

void IndirectFitPlotViewLegacy::setMinimumSpectrum(int minimum) {
  m_plotForm->spPlotSpectrum->setMinimum(minimum);
}

void IndirectFitPlotViewLegacy::setMaximumSpectrum(int maximum) {
  m_plotForm->spPlotSpectrum->setMaximum(maximum);
}

void IndirectFitPlotViewLegacy::setPlotSpectrum(int spectrum) {
  MantidQt::API::SignalBlocker blocker(m_plotForm->spPlotSpectrum);
  m_plotForm->spPlotSpectrum->setValue(spectrum);
}

void IndirectFitPlotViewLegacy::setBackgroundLevel(double value) {
  auto selector = m_topPlot->getSingleSelector("Background");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setPosition(value);
}

void IndirectFitPlotViewLegacy::setFitRange(double minimum, double maximum) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setRange(minimum, maximum);
}

void IndirectFitPlotViewLegacy::setFitRangeMinimum(double minimum) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setMinimum(minimum);
}

void IndirectFitPlotViewLegacy::setFitRangeMaximum(double maximum) {
  auto selector = m_topPlot->getRangeSelector("FitRange");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setMaximum(maximum);
}

void IndirectFitPlotViewLegacy::appendToDataSelection(const std::string &dataName) {
  MantidQt::API::SignalBlocker blocker(m_plotForm->cbDataSelection);
  m_plotForm->cbDataSelection->addItem(QString::fromStdString(dataName));
}

void IndirectFitPlotViewLegacy::setNameInDataSelection(const std::string &dataName,
                                                 std::size_t index) {
  m_plotForm->cbDataSelection->setItemText(boost::numeric_cast<int>(index),
                                           QString::fromStdString(dataName));
}

void IndirectFitPlotViewLegacy::clearDataSelection() {
  m_plotForm->cbDataSelection->clear();
}

void IndirectFitPlotViewLegacy::plotInTopPreview(
    const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
    std::size_t spectrum, Qt::GlobalColor colour) {
  m_topPlot->addSpectrum(name, workspace, spectrum, colour);
}

void IndirectFitPlotViewLegacy::plotInBottomPreview(
    const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
    std::size_t spectrum, Qt::GlobalColor colour) {
  m_bottomPlot->addSpectrum(name, workspace, spectrum, colour);
}

void IndirectFitPlotViewLegacy::removeFromTopPreview(const QString &name) {
  m_topPlot->removeSpectrum(name);
}

void IndirectFitPlotViewLegacy::removeFromBottomPreview(const QString &name) {
  m_bottomPlot->removeSpectrum(name);
}

void IndirectFitPlotViewLegacy::enablePlotGuess(bool enable) {
  if (!enable)
    m_plotForm->ckPlotGuess->setChecked(enable);
  m_plotForm->ckPlotGuess->setEnabled(enable);
}

void IndirectFitPlotViewLegacy::enableSpectrumSelection(bool enable) {
  if (!enable)
    m_plotForm->spPlotSpectrum->setValue(0);
  m_plotForm->spPlotSpectrum->setEnabled(enable);
}

void IndirectFitPlotViewLegacy::enableFitRangeSelection(bool enable) {
  m_topPlot->getRangeSelector("FitRange")->setVisible(enable);
}

void IndirectFitPlotViewLegacy::setFitSingleSpectrumText(QString const &text) {
  m_plotForm->pbFitSingle->setText(text);
}

void IndirectFitPlotViewLegacy::setFitSingleSpectrumEnabled(bool enable) {
  m_plotForm->pbFitSingle->setEnabled(enable);
}

void IndirectFitPlotViewLegacy::clearTopPreview() { m_topPlot->clear(); }

void IndirectFitPlotViewLegacy::clearBottomPreview() { m_bottomPlot->clear(); }

void IndirectFitPlotViewLegacy::clearPreviews() {
  clearTopPreview();
  clearBottomPreview();
}

void IndirectFitPlotViewLegacy::setHWHMRange(double minimum, double maximum) {
  auto selector = m_topPlot->getRangeSelector("HWHM");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setRange(minimum, maximum);
}

void IndirectFitPlotViewLegacy::setHWHMMaximum(double minimum) {
  auto selector = m_topPlot->getRangeSelector("HWHM");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setMaximum(minimum);
}

void IndirectFitPlotViewLegacy::setHWHMMinimum(double maximum) {
  auto selector = m_topPlot->getRangeSelector("HWHM");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setMinimum(maximum);
}

void IndirectFitPlotViewLegacy::addFitRangeSelector() {
  auto fitRangeSelector = m_topPlot->addRangeSelector("FitRange");

  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SIGNAL(startXChanged(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SIGNAL(endXChanged(double)));
}

void IndirectFitPlotViewLegacy::addBackgroundRangeSelector() {
  auto backRangeSelector =
      m_topPlot->addSingleSelector("Background", SingleSelector::YSINGLE);
  backRangeSelector->setVisible(false);
  backRangeSelector->setColour(Qt::darkGreen);
  backRangeSelector->setLowerBound(0.0);

  connect(backRangeSelector, SIGNAL(valueChanged(double)), this,
          SIGNAL(backgroundChanged(double)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  connect(backRangeSelector, SIGNAL(resetScientificBounds()), this,
          SLOT(setBackgroundBounds()));
#endif
}

void IndirectFitPlotViewLegacy::setBackgroundBounds() {
  auto backRangeSelector = m_topPlot->getSingleSelector("Background");
  backRangeSelector->setLowerBound(0.0);
}

void IndirectFitPlotViewLegacy::addHWHMRangeSelector() {
  auto hwhmRangeSelector = m_topPlot->addRangeSelector("HWHM");
  hwhmRangeSelector->setColour(Qt::red);
  hwhmRangeSelector->setRange(0.0, 0.0);
  hwhmRangeSelector->setVisible(false);

  connect(hwhmRangeSelector, SIGNAL(minValueChanged(double)), this,
          SIGNAL(hwhmMinimumChanged(double)));
  connect(hwhmRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SIGNAL(hwhmMaximumChanged(double)));
  connect(hwhmRangeSelector, SIGNAL(selectionChanged(double, double)), this,
          SIGNAL(hwhmChanged(double, double)));
}

void IndirectFitPlotViewLegacy::setBackgroundRangeVisible(bool visible) {
  m_topPlot->getSingleSelector("Background")->setVisible(visible);
}

void IndirectFitPlotViewLegacy::setHWHMRangeVisible(bool visible) {
  m_topPlot->getRangeSelector("HWHM")->setVisible(visible);
}

void IndirectFitPlotViewLegacy::displayMessage(const std::string &message) const {
  QMessageBox::information(parentWidget(), "MantidPlot - Warning",
                           QString::fromStdString(message));
}

void IndirectFitPlotViewLegacy::emitSelectedFitDataChanged(int index) {
  if (index >= 0)
    emit selectedFitDataChanged(boost::numeric_cast<std::size_t>(index));
}

// Required due to a bug in qt causing the valueChanged signal to be emitted
// twice due to the long amount of time taken to complete the necessary actions
void IndirectFitPlotViewLegacy::emitDelayedPlotSpectrumChanged() {
  QTimer::singleShot(150, this, SLOT(emitPlotSpectrumChanged()));
}

void IndirectFitPlotViewLegacy::emitPlotSpectrumChanged() {
  emit plotSpectrumChanged(
      boost::numeric_cast<std::size_t>(m_plotForm->spPlotSpectrum->value()));
}

void IndirectFitPlotViewLegacy::emitPlotSpectrumChanged(const QString &spectrum) {
  emit plotSpectrumChanged(spectrum.toULongLong());
}

void IndirectFitPlotViewLegacy::emitPlotGuessChanged(int doPlotGuess) {
  emit plotGuessChanged(doPlotGuess == Qt::Checked);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
