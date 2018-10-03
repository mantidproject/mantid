// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSpectrumSelectionPresenter.h"

#include "MantidKernel/ArrayProperty.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <algorithm>
#include <iterator>
#include <sstream>

#include <boost/numeric/conversion/cast.hpp>

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

struct SetViewSpectra : boost::static_visitor<> {
  explicit SetViewSpectra(IndirectSpectrumSelectionView *view) : m_view(view) {}

  void operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    m_view->displaySpectra(static_cast<int>(spectra.first),
                           static_cast<int>(spectra.second));
  }

  void operator()(const DiscontinuousSpectra<std::size_t> &spectra) const {
    m_view->displaySpectra(spectra.getString());
  }

private:
  IndirectSpectrumSelectionView *m_view;
};

std::string OR(const std::string &lhs, const std::string &rhs) {
  return "(" + lhs + "|" + rhs + ")";
}

std::string NATURAL_NUMBER(std::size_t digits) {
  return OR("0", "[1-9][0-9]{," + std::to_string(digits - 1) + "}");
}

namespace Regexes {
const std::string EMPTY = "^$";
const std::string SPACE = "[ ]*";
const std::string COMMA = SPACE + "," + SPACE;
const std::string MINUS = "\\-";

const std::string NUMBER = NATURAL_NUMBER(4);
const std::string NATURAL_RANGE = "(" + NUMBER + MINUS + NUMBER + ")";
const std::string NATURAL_OR_RANGE = OR(NATURAL_RANGE, NUMBER);
const std::string SPECTRA_LIST =
    "(" + NATURAL_OR_RANGE + "(" + COMMA + NATURAL_OR_RANGE + ")*)";

const std::string REAL_NUMBER = "(-?" + NUMBER + "(\\.[0-9]*)?)";
const std::string REAL_RANGE = "(" + REAL_NUMBER + COMMA + REAL_NUMBER + ")";
const std::string MASK_LIST =
    "(" + REAL_RANGE + "(" + COMMA + REAL_RANGE + ")*" + ")|" + EMPTY;
} // namespace Regexes
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSpectrumSelectionPresenter::IndirectSpectrumSelectionPresenter(
    IndirectFittingModel *model, IndirectSpectrumSelectionView *view)
    : QObject(nullptr), m_model(model), m_view(view), m_activeIndex(0),
      m_maskIndex(0) {
  connect(m_view.get(), SIGNAL(selectedSpectraChanged(const std::string &)),
          this, SLOT(updateSpectraList(const std::string &)));
  connect(m_view.get(), SIGNAL(selectedSpectraChanged(const std::string &)),
          this, SLOT(setMaskSpectraList(const std::string &)));
  connect(m_view.get(),
          SIGNAL(selectedSpectraChanged(std::size_t, std::size_t)), this,
          SLOT(updateSpectraRange(std::size_t, std::size_t)));

  connect(m_view.get(), SIGNAL(maskSpectrumChanged(int)), this,
          SLOT(setMaskIndex(int)));
  connect(m_view.get(), SIGNAL(maskSpectrumChanged(int)), this,
          SLOT(displayBinMask()));
  connect(m_view.get(), SIGNAL(maskChanged(const std::string &)), this,
          SLOT(setBinMask(const std::string &)));
  connect(m_view.get(), SIGNAL(maskChanged(const std::string &)), this,
          SIGNAL(maskChanged(const std::string &)));

  m_view->setSpectraRegex(Regexes::SPECTRA_LIST);
  m_view->setMaskBinsRegex(Regexes::MASK_LIST);
  m_view->setEnabled(false);
}

IndirectSpectrumSelectionPresenter::~IndirectSpectrumSelectionPresenter() {}

void IndirectSpectrumSelectionPresenter::disableView() {
  MantidQt::API::SignalBlocker<QObject> blocker(m_view.get());
  m_view->setDisabled(true);
}

void IndirectSpectrumSelectionPresenter::enableView() {
  m_view->setEnabled(true);
}

void IndirectSpectrumSelectionPresenter::setActiveIndexToZero() {
  setActiveModelIndex(0);
}

void IndirectSpectrumSelectionPresenter::updateSpectra() {
  const auto workspace = m_model->getWorkspace(m_activeIndex);
  if (workspace) {
    setSpectraRange(0, workspace->getNumberHistograms() - 1);
    const auto spectra = m_model->getSpectra(m_activeIndex);
    boost::apply_visitor(SetViewSpectra(m_view.get()), spectra);
    enableView();
  } else {
    m_view->clear();
    disableView();
  }
}

void IndirectSpectrumSelectionPresenter::setActiveModelIndex(
    std::size_t index) {
  m_activeIndex = index;
  updateSpectra();
}

void IndirectSpectrumSelectionPresenter::setSpectraRange(std::size_t minimum,
                                                         std::size_t maximum) {
  int minimumInt = boost::numeric_cast<int>(minimum);
  int maximumInt = boost::numeric_cast<int>(maximum);
  m_view->setSpectraRange(minimumInt, maximumInt);
}

void IndirectSpectrumSelectionPresenter::setModelSpectra(
    const Spectra &spectra) {
  try {
    m_model->setSpectra(spectra, m_activeIndex);
    m_spectraError.clear();
    m_view->hideSpectraErrorLabel();
    m_view->setMaskSelectionEnabled(true);
  } catch (const std::runtime_error &ex) {
    m_spectraError = ex.what();
    m_view->showSpectraErrorLabel();
    m_view->setMaskSelectionEnabled(false);
  }
}

void IndirectSpectrumSelectionPresenter::updateSpectraList(
    const std::string &spectraList) {
  setModelSpectra(DiscontinuousSpectra<std::size_t>(spectraList));
  emit spectraChanged(m_activeIndex);
}

void IndirectSpectrumSelectionPresenter::updateSpectraRange(
    std::size_t minimum, std::size_t maximum) {
  setModelSpectra(std::make_pair(minimum, maximum));
  emit spectraChanged(m_activeIndex);
}

void IndirectSpectrumSelectionPresenter::setMaskSpectraList(
    const std::string &spectra) {
  if (m_spectraError.empty())
    m_view->setMaskSpectraList(vectorFromString<std::size_t>(spectra));
  else
    m_view->setMaskSpectraList({});
}

void IndirectSpectrumSelectionPresenter::setBinMask(
    const std::string &maskString) {
  auto validator = validateMaskBinsString();

  if (validator.isAllInputValid()) {
    m_model->setExcludeRegion(maskString, m_activeIndex, m_maskIndex);
    m_view->hideMaskBinErrorLabel();
  } else {
    m_view->showMaskBinErrorLabel();
    emit invalidMaskBinsString(validator.generateErrorMessage());
  }
}

void IndirectSpectrumSelectionPresenter::setMaskIndex(int index) {
  if (index >= 0)
    m_maskIndex = boost::numeric_cast<std::size_t>(index);
}

void IndirectSpectrumSelectionPresenter::displayBinMask() {
  m_view->setMaskString(m_model->getExcludeRegion(m_activeIndex, m_maskIndex));
}

UserInputValidator &
IndirectSpectrumSelectionPresenter::validate(UserInputValidator &validator) {
  validator = validateSpectraString(validator);
  return m_view->validateMaskBinsString(validator);
}

UserInputValidator IndirectSpectrumSelectionPresenter::validateSpectraString() {
  UserInputValidator validator;
  return validateSpectraString(validator);
}

UserInputValidator &IndirectSpectrumSelectionPresenter::validateSpectraString(
    UserInputValidator &validator) {
  validator = m_view->validateSpectraString(validator);

  if (!m_spectraError.empty())
    validator.addErrorMessage(QString::fromStdString(m_spectraError));
  return validator;
}

UserInputValidator
IndirectSpectrumSelectionPresenter::validateMaskBinsString() {
  UserInputValidator uiv;
  return m_view->validateMaskBinsString(uiv);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
