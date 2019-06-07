// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPlotView.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <boost/numeric/conversion/cast.hpp>

#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitPlotView::IndirectFitPlotView(QWidget *parent)
    : IIndirectFitPlotView(parent), m_plotForm(new Ui::IndirectFitPreviewPlot) {
  m_plotForm->setupUi(this);

  connect(m_plotForm->cbDataSelection, SIGNAL(currentIndexChanged(int)), this,
          SLOT(emitSelectedFitDataChanged(int)));
  connect(m_plotForm->spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitPlotSpectrumChanged(int)));
  connect(m_plotForm->cbPlotSpectrum,
          SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(emitPlotSpectrumChanged(const QString &)));
  connect(m_plotForm->ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(emitPlotGuessChanged(int)));
  connect(m_plotForm->pbPlotPreview, SIGNAL(clicked()), this,
          SIGNAL(plotCurrentPreview()));
  connect(m_plotForm->pbFitSingle, SIGNAL(clicked()), this,
          SIGNAL(fitSelectedSpectrum()));

  m_plotForm->cbDataSelection->hide();
  addFitRangeSelector();
  addBackgroundRangeSelector();
  addHWHMRangeSelector();
}

IndirectFitPlotView::~IndirectFitPlotView() {}

std::string IndirectFitPlotView::getSpectrumText() const {
  return m_plotForm->cbPlotSpectrum->currentText().toStdString();
}

std::size_t IndirectFitPlotView::getSelectedSpectrum() const {
  if (m_plotForm->swPlotSpectrum->currentIndex() == 0)
    return m_plotForm->spPlotSpectrum->value();
  else if (m_plotForm->cbPlotSpectrum->count() != 0)
    return std::stoull(getSpectrumText());
  return 0;
}

int IndirectFitPlotView::getSelectedSpectrumIndex() const {
  if (m_plotForm->swPlotSpectrum->currentIndex() == 0)
    return m_plotForm->spPlotSpectrum->value() -
           m_plotForm->spPlotSpectrum->minimum();
  return m_plotForm->cbPlotSpectrum->currentIndex();
}

int IndirectFitPlotView::getSelectedDataIndex() const {
  return m_plotForm->cbDataSelection->currentIndex();
}

std::size_t IndirectFitPlotView::dataSelectionSize() const {
  return boost::numeric_cast<std::size_t>(m_plotForm->cbDataSelection->count());
}

bool IndirectFitPlotView::isPlotGuessChecked() const {
  return m_plotForm->ckPlotGuess->isChecked();
}

void IndirectFitPlotView::hideMultipleDataSelection() {
  m_plotForm->cbDataSelection->hide();
}

void IndirectFitPlotView::showMultipleDataSelection() {
  m_plotForm->cbDataSelection->show();
}

void IndirectFitPlotView::setAvailableSpectra(std::size_t minimum,
                                              std::size_t maximum) {
  m_plotForm->swPlotSpectrum->setCurrentIndex(0);
  m_plotForm->spPlotSpectrum->setMinimum(boost::numeric_cast<int>(minimum));
  m_plotForm->spPlotSpectrum->setMaximum(boost::numeric_cast<int>(maximum));
}

void IndirectFitPlotView::setAvailableSpectra(
    const std::vector<std::size_t>::const_iterator &from,
    const std::vector<std::size_t>::const_iterator &to) {
  m_plotForm->swPlotSpectrum->setCurrentIndex(1);
  m_plotForm->cbPlotSpectrum->clear();

  for (auto spectrum = from; spectrum < to; ++spectrum)
    m_plotForm->cbPlotSpectrum->addItem(QString::number(*spectrum));
}

void IndirectFitPlotView::setMinimumSpectrum(int minimum) {
  m_plotForm->spPlotSpectrum->setMinimum(minimum);
}

void IndirectFitPlotView::setMaximumSpectrum(int maximum) {
  m_plotForm->spPlotSpectrum->setMaximum(maximum);
}

void IndirectFitPlotView::setPlotSpectrum(int spectrum) {
  MantidQt::API::SignalBlocker blocker(m_plotForm->spPlotSpectrum);
  m_plotForm->spPlotSpectrum->setValue(spectrum);
}

void IndirectFitPlotView::setBackgroundLevel(double value) {
  auto selector = m_plotForm->ppPlotTop->getRangeSelector("Background");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setMinimum(value);
}

void IndirectFitPlotView::setFitRange(double minimum, double maximum) {
  auto selector = m_plotForm->ppPlotTop->getRangeSelector("FitRange");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setRange(minimum, maximum);
}

void IndirectFitPlotView::setFitRangeMinimum(double minimum) {
  auto selector = m_plotForm->ppPlotTop->getRangeSelector("FitRange");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setMinimum(minimum);
}

void IndirectFitPlotView::setFitRangeMaximum(double maximum) {
  auto selector = m_plotForm->ppPlotTop->getRangeSelector("FitRange");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setMaximum(maximum);
}

void IndirectFitPlotView::appendToDataSelection(const std::string &dataName) {
  MantidQt::API::SignalBlocker blocker(m_plotForm->cbDataSelection);
  m_plotForm->cbDataSelection->addItem(QString::fromStdString(dataName));
}

void IndirectFitPlotView::setNameInDataSelection(const std::string &dataName,
                                                 std::size_t index) {
  m_plotForm->cbDataSelection->setItemText(boost::numeric_cast<int>(index),
                                           QString::fromStdString(dataName));
}

void IndirectFitPlotView::clearDataSelection() {
  m_plotForm->cbDataSelection->clear();
}

void IndirectFitPlotView::plotInTopPreview(
    const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
    std::size_t spectrum, Qt::GlobalColor colour) {
  m_plotForm->ppPlotTop->addSpectrum(name, workspace, spectrum, colour);
}

void IndirectFitPlotView::plotInBottomPreview(
    const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
    std::size_t spectrum, Qt::GlobalColor colour) {
  m_plotForm->ppPlotBottom->addSpectrum(name, workspace, spectrum, colour);
}

void IndirectFitPlotView::removeFromTopPreview(const QString &name) {
  m_plotForm->ppPlotTop->removeSpectrum(name);
}

void IndirectFitPlotView::removeFromBottomPreview(const QString &name) {
  m_plotForm->ppPlotBottom->removeSpectrum(name);
}

void IndirectFitPlotView::enablePlotGuess(bool enable) {
  if (!enable)
    m_plotForm->ckPlotGuess->setChecked(enable);
  m_plotForm->ckPlotGuess->setEnabled(enable);
}

void IndirectFitPlotView::enableSpectrumSelection(bool enable) {
  if (!enable)
    m_plotForm->spPlotSpectrum->setValue(0);
  m_plotForm->spPlotSpectrum->setEnabled(enable);
}

void IndirectFitPlotView::enableFitRangeSelection(bool enable) {
  m_plotForm->ppPlotTop->getRangeSelector("FitRange")->setVisible(enable);
}

void IndirectFitPlotView::setFitSingleSpectrumText(QString const &text) {
  m_plotForm->pbFitSingle->setText(text);
}

void IndirectFitPlotView::setFitSingleSpectrumEnabled(bool enable) {
  m_plotForm->pbFitSingle->setEnabled(enable);
}

void IndirectFitPlotView::clearTopPreview() { m_plotForm->ppPlotTop->clear(); }

void IndirectFitPlotView::clearBottomPreview() {
  m_plotForm->ppPlotBottom->clear();
}

void IndirectFitPlotView::clear() {
  clearTopPreview();
  clearBottomPreview();
}

void IndirectFitPlotView::setHWHMRange(double minimum, double maximum) {
  auto selector = m_plotForm->ppPlotTop->getRangeSelector("HWHM");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setRange(minimum, maximum);
}

void IndirectFitPlotView::setHWHMMaximum(double minimum) {
  auto selector = m_plotForm->ppPlotTop->getRangeSelector("HWHM");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setMaximum(minimum);
}

void IndirectFitPlotView::setHWHMMinimum(double maximum) {
  auto selector = m_plotForm->ppPlotTop->getRangeSelector("HWHM");
  MantidQt::API::SignalBlocker blocker(selector);
  selector->setMinimum(maximum);
}

void IndirectFitPlotView::addFitRangeSelector() {
  auto fitRangeSelector = m_plotForm->ppPlotTop->addRangeSelector("FitRange");

  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SIGNAL(startXChanged(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SIGNAL(endXChanged(double)));
}

void IndirectFitPlotView::addBackgroundRangeSelector() {
  auto backRangeSelector = m_plotForm->ppPlotTop->addRangeSelector(
      "Background", MantidWidgets::RangeSelector::YSINGLE);
  backRangeSelector->setVisible(false);
  backRangeSelector->setColour(Qt::darkGreen);
  backRangeSelector->setRange(0.0, 1.0);

  connect(backRangeSelector, SIGNAL(minValueChanged(double)), this,
          SIGNAL(backgroundChanged(double)));
}

void IndirectFitPlotView::addHWHMRangeSelector() {
  auto hwhmRangeSelector = m_plotForm->ppPlotTop->addRangeSelector("HWHM");
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

void IndirectFitPlotView::setBackgroundRangeVisible(bool visible) {
  m_plotForm->ppPlotTop->getRangeSelector("Background")->setVisible(visible);
}

void IndirectFitPlotView::setHWHMRangeVisible(bool visible) {
  m_plotForm->ppPlotTop->getRangeSelector("HWHM")->setVisible(visible);
}

void IndirectFitPlotView::displayMessage(const std::string &message) const {
  QMessageBox::information(parentWidget(), "MantidPlot - Warning",
                           QString::fromStdString(message));
}

void IndirectFitPlotView::emitSelectedFitDataChanged(int index) {
  if (index >= 0)
    emit selectedFitDataChanged(boost::numeric_cast<std::size_t>(index));
}

void IndirectFitPlotView::emitPlotSpectrumChanged(int spectrum) {
  emit plotSpectrumChanged(boost::numeric_cast<std::size_t>(spectrum));
}

void IndirectFitPlotView::emitPlotSpectrumChanged(const QString &spectrum) {
  emit plotSpectrumChanged(spectrum.toULongLong());
}

void IndirectFitPlotView::emitPlotGuessChanged(int doPlotGuess) {
  emit plotGuessChanged(doPlotGuess == Qt::Checked);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
