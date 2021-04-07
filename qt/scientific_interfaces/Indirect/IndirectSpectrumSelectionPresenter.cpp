// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSpectrumSelectionPresenter.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Strings.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <algorithm>
#include <iterator>
#include <sstream>

#include <boost/numeric/conversion/cast.hpp>

namespace {
using namespace MantidQt::CustomInterfaces::IDA;
using namespace Mantid::Kernel::Strings;

struct SetViewSpectra {
  explicit SetViewSpectra(IndirectSpectrumSelectionView *view) : m_view(view) {}

  void operator()(const FunctionModelSpectra &spectra) const {
    if (spectra.isContinuous()) {
      m_view->displaySpectra(spectra.getMinMax());
    } else {
      m_view->displaySpectra(spectra.getString());
    }
  }

private:
  IndirectSpectrumSelectionView *m_view;
};

std::string OR(const std::string &lhs, const std::string &rhs) { return "(" + lhs + "|" + rhs + ")"; }

std::string NATURAL_NUMBER(std::size_t digits) { return OR("0", "[1-9][0-9]{," + std::to_string(digits - 1) + "}"); }

std::string constructSpectraString(std::vector<int> const &spectras) {
  return joinCompress(spectras.begin(), spectras.end());
}

std::vector<std::string> splitStringBy(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  subStrings.erase(std::remove_if(subStrings.begin(), subStrings.end(),
                                  [](std::string const &subString) { return subString.empty(); }),
                   subStrings.end());
  return subStrings;
}

std::string getSpectraRange(std::string const &string) {
  auto const bounds = splitStringBy(string, "-");
  return std::stoull(bounds[0]) > std::stoull(bounds[1]) ? bounds[1] + "-" + bounds[0] : string;
}

std::string rearrangeSpectraSubString(std::string const &string) {
  return string.find("-") != std::string::npos ? getSpectraRange(string) : string;
}

// Swaps the two numbers in a spectra range if they go from large to small
std::string rearrangeSpectraRangeStrings(std::string const &string) {
  std::string spectraString;
  std::vector<std::string> subStrings = splitStringBy(string, ",");
  for (auto it = subStrings.begin(); it < subStrings.end(); ++it) {
    spectraString += rearrangeSpectraSubString(*it);
    spectraString += it != subStrings.end() ? "," : "";
  }
  return spectraString;
}

std::string createSpectraString(std::string string) {
  string.erase(std::remove_if(string.begin(), string.end(), isspace), string.end());
  std::vector<int> spectras = parseRange(rearrangeSpectraRangeStrings(string));
  std::sort(spectras.begin(), spectras.end());
  // Remove duplicate entries
  spectras.erase(std::unique(spectras.begin(), spectras.end()), spectras.end());
  return constructSpectraString(spectras);
}

namespace Regexes {
const std::string EMPTY = "^$";
const std::string SPACE = "[ ]*";
const std::string COMMA = SPACE + "," + SPACE;
const std::string MINUS = "\\-";

const std::string NUMBER = NATURAL_NUMBER(4);
const std::string NATURAL_RANGE = "(" + NUMBER + MINUS + NUMBER + ")";
const std::string NATURAL_OR_RANGE = OR(NATURAL_RANGE, NUMBER);
const std::string SPECTRA_LIST = "(" + NATURAL_OR_RANGE + "(" + COMMA + NATURAL_OR_RANGE + ")*)";

const std::string REAL_NUMBER = "(-?" + NUMBER + "(\\.[0-9]*)?)";
const std::string REAL_RANGE = "(" + REAL_NUMBER + COMMA + REAL_NUMBER + ")";
const std::string MASK_LIST = "(" + REAL_RANGE + "(" + COMMA + REAL_RANGE + ")*" + ")|" + EMPTY;
} // namespace Regexes
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSpectrumSelectionPresenter::IndirectSpectrumSelectionPresenter(IndirectFittingModel *model,
                                                                       IndirectSpectrumSelectionView *view)
    : QObject(nullptr), m_model(model), m_view(view), m_activeIndex{0}, m_maskIndex{0} {
  connect(m_view.get(), SIGNAL(selectedSpectraChanged(const std::string &)), this,
          SLOT(updateSpectraList(const std::string &)));
  connect(m_view.get(), SIGNAL(selectedSpectraChanged(const std::string &)), this,
          SLOT(setMaskSpectraList(const std::string &)));
  connect(m_view.get(), SIGNAL(selectedSpectraChanged(WorkspaceIndex, WorkspaceIndex)), this,
          SLOT(updateSpectraRange(WorkspaceIndex, WorkspaceIndex)));
  connect(m_view.get(), SIGNAL(selectedSpectraChanged(const std::string &)), this,
          SLOT(displaySpectraList(const std::string &)));

  connect(m_view.get(), SIGNAL(maskSpectrumChanged(WorkspaceIndex)), this, SLOT(setMaskIndex(WorkspaceIndex)));
  connect(m_view.get(), SIGNAL(maskSpectrumChanged(WorkspaceIndex)), this, SLOT(displayBinMask()));
  connect(m_view.get(), SIGNAL(maskChanged(const std::string &)), this, SLOT(setBinMask(const std::string &)));
  connect(m_view.get(), SIGNAL(maskChanged(const std::string &)), this, SLOT(displayBinMask()));
  connect(m_view.get(), SIGNAL(maskChanged(const std::string &)), this, SIGNAL(maskChanged(const std::string &)));
  connect(m_view.get(), SIGNAL(spectraSelectionWidgetChanged(int)), this, SLOT(initSpectraSelectionWidget(int)));

  m_view->setSpectraRegex(Regexes::SPECTRA_LIST);
  m_view->setMaskBinsRegex(Regexes::MASK_LIST);
  m_view->setEnabled(false);
}

IndirectSpectrumSelectionPresenter::~IndirectSpectrumSelectionPresenter() {}

void IndirectSpectrumSelectionPresenter::disableView() {
  MantidQt::API::SignalBlocker blocker(m_view.get());
  m_view->setDisabled(true);
}

void IndirectSpectrumSelectionPresenter::enableView() { m_view->setEnabled(true); }

void IndirectSpectrumSelectionPresenter::initSpectraSelectionWidget(int index) {
  auto spectra = m_model->getSpectra(m_activeIndex);
  if (index == 0) {
    m_view->displaySpectra(spectra.getMinMax());
  } else {
    m_view->displaySpectra(spectra.getString());
  }
}

void IndirectSpectrumSelectionPresenter::setActiveIndexToZero() { setActiveModelIndex(TableDatasetIndex{0}); }

void IndirectSpectrumSelectionPresenter::updateSpectra() {
  const auto ws = m_model->getWorkspace(m_activeIndex);
  if (ws) {
    const auto spectra = m_model->getSpectra(m_activeIndex);
    setSpectraRange(spectra.front(), spectra.back());
    SetViewSpectra(m_view.get())(spectra);
    enableView();
  } else {
    m_view->clear();
    disableView();
  }
}

void IndirectSpectrumSelectionPresenter::setActiveModelIndex(TableDatasetIndex index) {
  m_activeIndex = index;
  updateSpectra();
}

void IndirectSpectrumSelectionPresenter::setSpectraRange(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  m_view->setSpectraRange(minimum, maximum);
}

void IndirectSpectrumSelectionPresenter::setModelSpectra(FunctionModelSpectra const &spectra) {
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

void IndirectSpectrumSelectionPresenter::updateSpectraList(std::string const &spectraList) {
  setModelSpectra(FunctionModelSpectra(createSpectraString(spectraList)));
  emit spectraChanged(m_activeIndex);
}

void IndirectSpectrumSelectionPresenter::updateSpectraRange(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  setModelSpectra(FunctionModelSpectra(minimum, maximum));
  emit spectraChanged(m_activeIndex);
}

void IndirectSpectrumSelectionPresenter::setMaskSpectraList(std::string const &spectra) {
  if (m_spectraError.empty()) {
    auto const intVec = vectorFromString<int>(spectra);
    std::vector<WorkspaceIndex> vec(intVec.size());
    std::transform(intVec.begin(), intVec.end(), vec.begin(), [](size_t i) { return WorkspaceIndex{i}; });
    m_view->setMaskSpectraList(vec);
  } else
    m_view->setMaskSpectraList({});
}

void IndirectSpectrumSelectionPresenter::displaySpectraList(std::string const &spectra) {
  m_view->displaySpectra(createSpectraString(spectra));
}

void IndirectSpectrumSelectionPresenter::setBinMask(std::string const &maskString) {
  auto validator = validateMaskBinsString();

  if (validator.isAllInputValid()) {
    m_model->setExcludeRegion(maskString, m_activeIndex, m_maskIndex);
    m_view->hideMaskBinErrorLabel();
  } else {
    m_view->showMaskBinErrorLabel();
    emit invalidMaskBinsString(validator.generateErrorMessage());
  }
}

void IndirectSpectrumSelectionPresenter::setMaskIndex(WorkspaceIndex index) { m_maskIndex = index; }

void IndirectSpectrumSelectionPresenter::displayBinMask() {
  m_view->setMaskString(m_model->getExcludeRegion(m_activeIndex, m_maskIndex));
}

UserInputValidator &IndirectSpectrumSelectionPresenter::validate(UserInputValidator &validator) {
  validator = validateSpectraString(validator);
  return m_view->validateMaskBinsString(validator);
}

UserInputValidator IndirectSpectrumSelectionPresenter::validateSpectraString() {
  UserInputValidator validator;
  return validateSpectraString(validator);
}

UserInputValidator &IndirectSpectrumSelectionPresenter::validateSpectraString(UserInputValidator &validator) {
  validator = m_view->validateSpectraString(validator);

  if (!m_spectraError.empty())
    validator.addErrorMessage(QString::fromStdString(m_spectraError));
  return validator;
}

UserInputValidator IndirectSpectrumSelectionPresenter::validateMaskBinsString() {
  UserInputValidator uiv;
  return m_view->validateMaskBinsString(uiv);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
