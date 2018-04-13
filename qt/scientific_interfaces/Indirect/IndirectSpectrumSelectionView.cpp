#include "IndirectSpectrumSelectionView.h"

#include <QRegExpValidator>

#include <boost/numeric/conversion/cast.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSpectrumSelectionView::IndirectSpectrumSelectionView(QWidget *parent)
    : API::MantidWidget(parent), m_selector(new Ui::IndirectSpectrumSelector) {
  m_selector->setupUi(this);

  connect(m_selector->spMaximumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitSpectraRangeChanged()));
  connect(m_selector->spMinimumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitSpectraRangeChanged()));
  connect(m_selector->leSpectra, SIGNAL(textChanged(const QString &)), this,
          SLOT(emitSpectraStringChanged()));

  connect(m_selector->spMaskSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitMaskSpectrumChanged(int)));
  connect(m_selector->cbMaskSpectrum,
          SIGNAL(currentTextChanged(const QString &)), this,
          SLOT(emitMaskSpectrumChanged(const QString &)));
  connect(m_selector->leMaskBins, SIGNAL(textChanged(const QString &)), this,
          SLOT(emitMaskChanged(const QString &)));

  connect(m_selector->spMaximumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSpectraRangeMaxiMin(int)));
  connect(m_selector->spMinimumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSpectraRangeMiniMax(int)));

  connect(m_selector->cbSelectionMode, SIGNAL(currentIndexChanged(int)), this,
          SLOT(emitSpectraChanged()));
}

IndirectSpectrumSelectionView::~IndirectSpectrumSelectionView() {}

SpectrumSelectionMode IndirectSpectrumSelectionView::selectionMode() const {
  return static_cast<SpectrumSelectionMode>(
      m_selector->swSpectraSelection->currentIndex());
}

std::size_t IndirectSpectrumSelectionView::minimumSpectrum() const {
  return boost::numeric_cast<std::size_t>(
      m_selector->spMinimumSpectrum->value());
}

std::size_t IndirectSpectrumSelectionView::maximumSpectrum() const {
  return boost::numeric_cast<std::size_t>(
      m_selector->spMaximumSpectrum->value());
}

std::size_t IndirectSpectrumSelectionView::selectedMaskSpectrum() const {
  if (selectionMode() == SpectrumSelectionMode::RANGE)
    return boost::numeric_cast<std::size_t>(
        m_selector->spMaskSpectrum->value());
  else
    return boost::numeric_cast<std::size_t>(
        m_selector->cbMaskSpectrum->currentText().toInt());
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

void IndirectSpectrumSelectionView::displaySpectra(int minimum, int maximum) {
  setMinimumSpectrum(minimum);
  setMaximumSpectrum(maximum);
  m_selector->cbSelectionMode->setCurrentIndex(
      static_cast<int>(SpectrumSelectionMode::RANGE));
}

void IndirectSpectrumSelectionView::setSpectraRange(int minimum, int maximum) {
  m_selector->spMinimumSpectrum->setMinimum(minimum);
  m_selector->spMaximumSpectrum->setMaximum(maximum);
}

void IndirectSpectrumSelectionView::setMaskSpectraRange(int minimum,
                                                        int maximum) {
  m_selector->spMaskSpectrum->setMinimum(minimum);
  m_selector->spMaskSpectrum->setMaximum(maximum);
}

void IndirectSpectrumSelectionView::setMaskSpectraList(
    const std::vector<std::size_t> &spectra) {
  m_selector->cbMaskSpectrum->clear();
  for (const auto &spectrum : spectra)
    m_selector->cbMaskSpectrum->addItem(QString::number(spectrum));
}

void IndirectSpectrumSelectionView::setSpectraRegex(const std::string &regex) {
  m_selector->leSpectra->setValidator(
      createValidator(QString::fromStdString(regex)));
}

void IndirectSpectrumSelectionView::setMaskBinsRegex(const std::string &regex) {
  m_selector->leMaskBins->setValidator(
      createValidator(QString::fromStdString(regex)));
}

void IndirectSpectrumSelectionView::setMinimumSpectrum(std::size_t spectrum) {
  m_selector->spMinimumSpectrum->setValue(boost::numeric_cast<int>(spectrum));
}

void IndirectSpectrumSelectionView::setMaximumSpectrum(std::size_t spectrum) {
  m_selector->spMinimumSpectrum->setValue(boost::numeric_cast<int>(spectrum));
}

void IndirectSpectrumSelectionView::setMaskSpectrum(std::size_t spectrum) {
  m_selector->spMaskSpectrum->setValue(boost::numeric_cast<int>(spectrum));
}

void IndirectSpectrumSelectionView::setSpectraString(
    const std::string &spectraString) {
  m_selector->leSpectra->setText(QString::fromStdString(spectraString));
}

void IndirectSpectrumSelectionView::setMaskString(
    const std::string &maskString) {
  m_selector->leMaskBins->setText(QString::fromStdString(maskString));
}

void IndirectSpectrumSelectionView::setSpectraRangeMaxiMin(int value) {
  m_selector->spMinimumSpectrum->setMaximum(value);
  m_selector->spMaskSpectrum->setMaximum(value);
}

void IndirectSpectrumSelectionView::setSpectraRangeMiniMax(int value) {
  m_selector->spMaximumSpectrum->setMinimum(value);
  m_selector->spMaskSpectrum->setMinimum(value);
}

void IndirectSpectrumSelectionView::showSpectraErrorLabel() {
  UserInputValidator().setErrorLabel(m_selector->lbSpectraError, true);
}

void IndirectSpectrumSelectionView::showMaskBinErrorLabel() {
  UserInputValidator().setErrorLabel(m_selector->lbMaskBinsError, true);
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

void IndirectSpectrumSelectionView::emitSpectraChanged(int modeIndex) {
  const auto selectionMode = static_cast<SpectrumSelectionMode>(modeIndex);
  if (selectionMode == SpectrumSelectionMode::RANGE)
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

void IndirectSpectrumSelectionView::emitMaskSpectrumChanged(int spectrum) {
  emit maskSpectrumChanged(boost::numeric_cast<std::size_t>(spectrum));
}

void IndirectSpectrumSelectionView::emitMaskSpectrumChanged(
    const QString &spectrum) {
  emit maskSpectrumChanged(
      boost::numeric_cast<std::size_t>(spectrum.toULong()));
}

void IndirectSpectrumSelectionView::emitMaskChanged(const QString &mask) {
  emit maskChanged(selectedMaskSpectrum(), mask.toStdString());
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
