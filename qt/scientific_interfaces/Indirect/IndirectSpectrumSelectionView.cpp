// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSpectrumSelectionView.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <QRegExpValidator>

#include <boost/numeric/conversion/cast.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSpectrumSelectionView::IndirectSpectrumSelectionView(QWidget *parent)
    : API::MantidWidget(parent), m_selector(new Ui::IndirectSpectrumSelector) {
  m_selector->setupUi(this);

  connect(m_selector->cbMaskSpectrum, SIGNAL(currentIndexChanged(int)), this,
          SLOT(enableMaskLineEdit(int)));

  connect(m_selector->spMaximumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSpectraRangeMaxiMin(int)));
  connect(m_selector->spMinimumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSpectraRangeMiniMax(int)));

  connect(m_selector->spMaximumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitSpectraRangeChanged()));
  connect(m_selector->spMinimumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitSpectraRangeChanged()));
  connect(m_selector->leSpectra, SIGNAL(editingFinished()), this,
          SLOT(emitSpectraStringChanged()));

  connect(m_selector->spMaskSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitMaskSpectrumChanged(int)));
  connect(m_selector->cbMaskSpectrum,
          SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(emitMaskSpectrumChanged(const QString &)));
  connect(m_selector->leMaskBins, SIGNAL(editingFinished()), this,
          SLOT(emitMaskChanged()));

  connect(m_selector->cbSelectionMode, SIGNAL(currentIndexChanged(int)), this,
          SLOT(emitSpectraChanged(int)));
  connect(m_selector->cbSelectionMode, SIGNAL(currentIndexChanged(int)), this,
          SLOT(clearMaskString()));

  connect(m_selector->swSpectraSelection, SIGNAL(currentChanged(int)), this, SIGNAL(spectraSelectionWidgetChanged(int)));
}

IndirectSpectrumSelectionView::~IndirectSpectrumSelectionView() {}

SpectrumSelectionMode IndirectSpectrumSelectionView::selectionMode() const {
  return static_cast<SpectrumSelectionMode>(
      m_selector->swSpectraSelection->currentIndex());
}

WorkspaceIndex IndirectSpectrumSelectionView::minimumSpectrum() const {
  return WorkspaceIndex{m_selector->spMinimumSpectrum->value()};
}

WorkspaceIndex IndirectSpectrumSelectionView::maximumSpectrum() const {
  return WorkspaceIndex{m_selector->spMaximumSpectrum->value()};
}

std::string IndirectSpectrumSelectionView::spectraString() const {
  return m_selector->leSpectra->text().toStdString();
}

std::string IndirectSpectrumSelectionView::maskString() const {
  return m_selector->leMaskBins->text().toStdString();
}

void IndirectSpectrumSelectionView::displaySpectra(
    const std::string &spectraString) {
  setSpectraString(spectraString);
  m_selector->cbSelectionMode->setCurrentIndex(
      static_cast<int>(SpectrumSelectionMode::STRING));
}

void IndirectSpectrumSelectionView::displaySpectra(std::pair<WorkspaceIndex, WorkspaceIndex> minmax) {
  setMinimumSpectrum(minmax.first);
  setMaximumSpectrum(minmax.second);
  m_selector->cbSelectionMode->setCurrentIndex(
      static_cast<int>(SpectrumSelectionMode::RANGE));
}

void IndirectSpectrumSelectionView::setSpectraRange(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  setSpectraRangeMinimum(minimum);
  setSpectraRangeMaximum(maximum);
}

void IndirectSpectrumSelectionView::setSpectraRangeMinimum(WorkspaceIndex minimum) {
  MantidQt::API::SignalBlocker blocker(m_selector->spMinimumSpectrum);
  m_selector->spMinimumSpectrum->setMinimum(minimum.value);
  setSpectraRangeMiniMax(minimum.value);
}

void IndirectSpectrumSelectionView::setSpectraRangeMaximum(WorkspaceIndex maximum) {
  MantidQt::API::SignalBlocker blocker(m_selector->spMaximumSpectrum);
  m_selector->spMaximumSpectrum->setMaximum(maximum.value);
  setSpectraRangeMaxiMin(maximum.value);
}

void IndirectSpectrumSelectionView::setMaskSpectraList(
    const std::vector<WorkspaceIndex> &spectra) {
  m_selector->cbMaskSpectrum->clear();
  for (const auto &spectrum : spectra)
    m_selector->cbMaskSpectrum->addItem(QString::number(spectrum.value));
}

void IndirectSpectrumSelectionView::setMaskSelectionEnabled(bool enabled) {
  m_selector->cbMaskSpectrum->setEnabled(enabled);
  m_selector->lbMaskSpectrum->setEnabled(enabled);
  m_selector->leMaskBins->setEnabled(enabled);
}

void IndirectSpectrumSelectionView::clear() {
  m_selector->leSpectra->clear();
  m_selector->leMaskBins->clear();
  m_selector->cbMaskSpectrum->clear();
  setSpectraRange(WorkspaceIndex{0}, WorkspaceIndex{0});
}

void IndirectSpectrumSelectionView::setSpectraRegex(const std::string &regex) {
  m_selector->leSpectra->setValidator(
      createValidator(QString::fromStdString(regex)));
}

void IndirectSpectrumSelectionView::setMaskBinsRegex(const std::string &regex) {
  m_selector->leMaskBins->setValidator(
      createValidator(QString::fromStdString(regex)));
}

void IndirectSpectrumSelectionView::setMinimumSpectrum(WorkspaceIndex spectrum) {
  MantidQt::API::SignalBlocker blocker(m_selector->spMinimumSpectrum);
  m_selector->spMinimumSpectrum->setValue(boost::numeric_cast<int>(spectrum.value));
}

void IndirectSpectrumSelectionView::setMaximumSpectrum(WorkspaceIndex spectrum) {
  MantidQt::API::SignalBlocker blocker(m_selector->spMaximumSpectrum);
  m_selector->spMaximumSpectrum->setValue(boost::numeric_cast<int>(spectrum.value));
}

void IndirectSpectrumSelectionView::setMaskSpectrum(WorkspaceIndex spectrum) {
  MantidQt::API::SignalBlocker blocker(m_selector->spMaskSpectrum);
  m_selector->spMaskSpectrum->setValue(boost::numeric_cast<int>(spectrum.value));
}

void IndirectSpectrumSelectionView::setSpectraString(
    const std::string &spectraString) {
  MantidQt::API::SignalBlocker blocker(m_selector->leSpectra);
  m_selector->leSpectra->setText(QString::fromStdString(spectraString));
}

void IndirectSpectrumSelectionView::setMaskString(
    const std::string &maskString) {
  MantidQt::API::SignalBlocker blocker(m_selector->leMaskBins);
  m_selector->leMaskBins->setText(QString::fromStdString(maskString));
}

void IndirectSpectrumSelectionView::setSpectraRangeMaxiMin(int value) {
  MantidQt::API::SignalBlocker blocker(m_selector->spMinimumSpectrum);
  m_selector->spMinimumSpectrum->setMaximum(value);
  m_selector->spMaskSpectrum->setMaximum(value);
}

void IndirectSpectrumSelectionView::setSpectraRangeMiniMax(int value) {
  MantidQt::API::SignalBlocker blocker(m_selector->spMaximumSpectrum);
  m_selector->spMaximumSpectrum->setMinimum(value);
  m_selector->spMaskSpectrum->setMinimum(value);
}

void IndirectSpectrumSelectionView::showSpectraErrorLabel() {
  UserInputValidator().setErrorLabel(m_selector->lbSpectraError, false);
}

void IndirectSpectrumSelectionView::showMaskBinErrorLabel() {
  UserInputValidator().setErrorLabel(m_selector->lbMaskBinsError, false);
}

void IndirectSpectrumSelectionView::hideSpectraErrorLabel() {
  m_selector->lbSpectraError->setText("");
  m_selector->lbSpectraError->setVisible(false);
}

void IndirectSpectrumSelectionView::hideMaskBinErrorLabel() {
  m_selector->lbMaskBinsError->setText("");
  m_selector->lbMaskBinsError->setVisible(false);
}

QValidator *
IndirectSpectrumSelectionView::createValidator(const QString &regex) {
  return new QRegExpValidator(QRegExp(regex), this);
}

UserInputValidator &IndirectSpectrumSelectionView::validateSpectraString(
    UserInputValidator &uiv) const {
  if (selectionMode() == SpectrumSelectionMode::STRING)
    uiv.checkFieldIsValid("Spectra", m_selector->leSpectra,
                          m_selector->lbSpectraError);
  return uiv;
}

UserInputValidator &IndirectSpectrumSelectionView::validateMaskBinsString(
    UserInputValidator &uiv) const {
  uiv.checkFieldIsValid("Mask Bins", m_selector->leMaskBins,
                        m_selector->lbMaskBinsError);
  return uiv;
}

void IndirectSpectrumSelectionView::hideSpectrumSelector() {
  m_selector->lbSelectionMode->hide();
  m_selector->cbSelectionMode->hide();
  m_selector->swSpectraSelection->hide();
  m_selector->lbColon->hide();
}

void IndirectSpectrumSelectionView::showSpectrumSelector() {
  m_selector->lbSelectionMode->show();
  m_selector->cbSelectionMode->show();
  m_selector->swSpectraSelection->show();
  m_selector->lbColon->show();
}

void IndirectSpectrumSelectionView::hideMaskSpectrumSelector() {
  m_selector->swMaskSpectrumSelection->hide();
}

void IndirectSpectrumSelectionView::showMaskSpectrumSelector() {
  m_selector->swMaskSpectrumSelection->show();
}

void IndirectSpectrumSelectionView::clearMaskString() {
  m_selector->leMaskBins->clear();
}

void IndirectSpectrumSelectionView::enableMaskLineEdit(int doEnable) {
  if (doEnable >= 0 || selectionMode() == SpectrumSelectionMode::RANGE)
    m_selector->leMaskBins->setEnabled(true);
  else
    m_selector->leMaskBins->setEnabled(false);
}

void IndirectSpectrumSelectionView::emitSpectraChanged(int modeIndex) {
  const auto mode = static_cast<SpectrumSelectionMode>(modeIndex);
  if (mode == SpectrumSelectionMode::RANGE)
    emitSpectraRangeChanged();
  else
    emitSpectraStringChanged();
}

void IndirectSpectrumSelectionView::emitSpectraRangeChanged() {
  emit selectedSpectraChanged(minimumSpectrum(), maximumSpectrum());
}

void IndirectSpectrumSelectionView::emitSpectraStringChanged() {
  emit selectedSpectraChanged(m_selector->leSpectra->text().toStdString());
}

void IndirectSpectrumSelectionView::emitMaskChanged() {
  emit maskChanged(m_selector->leMaskBins->text().toStdString());
}

void IndirectSpectrumSelectionView::emitMaskSpectrumChanged(
    const QString &spectrum) {
  emit maskSpectrumChanged(WorkspaceIndex{spectrum.toInt()});
}

void IndirectSpectrumSelectionView::emitMaskSpectrumChanged(int spectrum)
{
  emit maskSpectrumChanged(WorkspaceIndex{spectrum});
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
