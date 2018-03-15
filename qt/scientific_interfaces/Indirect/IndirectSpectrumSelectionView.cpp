#include "IndirectSpectrumSelectionView.h"

#include <QRegExpValidator>

#include <boost/numeric/conversion/cast.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSpectrumSelectionView::IndirectSpectrumSelectionView(
    Ui::IndirectSpectrumSelector *selector)
    : QObject(nullptr), m_selector(selector) {
  connect(m_selector->spMaximumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitSpectraRangeChanged()));
  connect(m_selector->spMinimumSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitSpectraRangeChanged()));
  connect(m_selector->leSpectra, SIGNAL(textChanged(const QString &)), this,
          SLOT(emitSpectraStringChanged()));

  connect(m_selector->spMaskSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(emitMaskSpectrumChanged(int)));
  connect(m_selector->leMaskBins, SIGNAL(textChanged(const QString &)), this,
          SLOT(emitMaskChanged(const QString &)));
}

IndirectSpectrumSelectionView::~IndirectSpectrumSelectionView() {}

SpectrumSelectionMode IndirectSpectrumSelectionView::selectionMode() const {
  return SpectrumSelectionMode::RANGE;
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
  return boost::numeric_cast<std::size_t>(m_selector->spMaskSpectrum->value());
}

std::string IndirectSpectrumSelectionView::spectraString() const {
  return m_selector->leSpectra->text().toStdString();
}

std::string IndirectSpectrumSelectionView::maskString() const {
  return m_selector->leMaskBins->text().toStdString();
}

void IndirectSpectrumSelectionView::setSpectrumRange(std::size_t minimum,
                                                     std::size_t maximum) {
  m_selector->spMinimumSpectrum->setMinimum(minimum);
  m_selector->spMaximumSpectrum->setMaximum(maximum);
}

void IndirectSpectrumSelectionView::setMaskSpectrumRange(std::size_t minimum,
                                                         std::size_t maximum) {
  m_selector->spMaskSpectrum->setMinimum(minimum);
  m_selector->spMaskSpectrum->setMaximum(maximum);
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
  uiv.checkFieldIsValid("Spectra", m_selector->leSpectra,
                        m_selector->lbSpectraError);
  return uiv;
}

UserInputValidator &IndirectSpectrumSelectionView::validateMaskBinsString(
    UserInputValidator &uiv) const {
  uiv.checkFieldIsValid("Mask Bins", m_selector->leSpectra,
                        m_selector->lbMaskBinsError);
  return uiv;
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

void IndirectSpectrumSelectionView::emitMaskChanged(const QString &mask) {
  emit maskChanged(selectedMaskSpectrum(), mask.toStdString());
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
