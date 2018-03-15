#include "IndirectSpectrumSelectionPresenter.h"

#include "MantidKernel/ArrayProperty.h"

#include <algorithm>
#include <sstream>

namespace {

template <typename T>
std::vector<T> vectorFromString(const std::string &listString) {
  try {
    return Mantid::Kernel::ArrayProperty<T>("vector", listString);
  } catch (const std::runtime_error &) {
    return std::vector<T>();
  }
}

template <template <class...> typename Map, typename Key, typename Value,
          typename... Ts>
const Value &getValueOr(const Map<Key, Value, Ts...> &map, const Key &key,
                        const Value &value) {
  const auto iterator = map.find(key);
  return iterator == map.end() ? value : iterator->second;
}

template <typename T, typename... Ts>
std::vector<T, Ts...> outOfRange(const std::vector<T, Ts...> &values,
                                 const T &minimum, const T &maximum) {
  std::vector<T, Ts...> result;
  for (auto &&value : values) {
    if (value < minimum || value > maximum)
      result.emplace_back(value);
  }
  return result;
}

template <typename T>
std::string commaSeparatedList(const std::vector<T> &values) {
  std::stringstream stream;
  std::copy(values.begin(), values.end(),
            std::ostream_iterator<T>(stream, ","));
  return stream.str();
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

namespace Regexes {
const std::string SPACE = "(\\s*)";
const std::string COMMA = "(" + SPACE + "," + SPACE + ")";
const std::string MINUS = "(" + SPACE + "\\-" + SPACE + ")";
const std::string NUMBER = "([1-9][0-9]*)";
const std::string RANGE = "(" + NUMBER + MINUS + NUMBER + ")";
const std::string NUMBER_OR_RANGE = "(" + NUMBER + "|" + RANGE + ")";
const std::string LIST = "(" + NUMBER_OR_RANGE + "(" + NUMBER_OR_RANGE + ")*)";
const std::string RANGE_LIST = "(" + RANGE + "(" + RANGE + ")*)";
} // namespace Regexes

IndirectSpectrumSelectionPresenter::IndirectSpectrumSelectionPresenter(
    IndirectSpectrumSelectionView *view)
    : QObject(nullptr), m_view(view) {
  connect(m_view.get(), SIGNAL(selectedSpectraChanged(const std::string &)),
          this, SLOT(updateSpectraList(const std::string &)));
  connect(m_view.get(),
          SIGNAL(selectedSpectraChanged(std::size_t, std::size_t)), this,
          SLOT(updateSpectraRange(std::size_t, std::size_t)));
  connect(m_view.get(), SIGNAL(maskChanged(std::size_t, const std::string &)),
          this, SLOT(setBinMask(std::size_t, const std::string &)));
  connect(m_view.get(), SIGNAL(maskSpectrumChanged(std::size_t)), this,
          SLOT(displayBinMask(std::size_t index)));

  connect(m_view.get(), SIGNAL(maskChanged(std::size_t, const std::string &)),
          this, SIGNAL(maskChanged(std::size_t, const std::string &)));

  m_view->setSpectraRegex(Regexes::LIST);
  m_view->setMaskBinsRegex(Regexes::RANGE_LIST);
}

IndirectSpectrumSelectionPresenter::~IndirectSpectrumSelectionPresenter() {}

std::pair<std::size_t, std::size_t>
IndirectSpectrumSelectionPresenter::spectraRange() const {
  return std::make_pair(m_minimumSpectrum, m_maximumSpectrum);
}

Spectra IndirectSpectrumSelectionPresenter::selectedSpectra() const {
  return selectedSpectra(m_view->selectionMode());
}

Spectra IndirectSpectrumSelectionPresenter::selectedSpectra(
    SpectrumSelectionMode mode) const {
  if (mode == SpectrumSelectionMode::RANGE)
    return m_spectraRange;
  else
    return m_spectraList;
}

const std::unordered_map<std::size_t, std::string> &
IndirectSpectrumSelectionPresenter::binMasks() const {
  return m_binMasks;
}

void IndirectSpectrumSelectionPresenter::setSpectrumRange(std::size_t minimum,
                                                          std::size_t maximum) {
  m_minimumSpectrum = minimum;
  m_maximumSpectrum = maximum;
  m_view->setSpectrumRange(minimum, maximum);
  m_view->setMaskSpectrumRange(minimum, maximum);
}

void IndirectSpectrumSelectionPresenter::setSpectraList(
    const std::string &spectraList) {
  m_spectraList = vectorFromString<std::size_t>(m_view->spectraString());
  auto validator = validateSpectraString();

  if (!validator.isAllInputValid()) {
    m_spectraList.clear();
    emit invalidSpectraString(validator.generateErrorMessage());
  }
}

void IndirectSpectrumSelectionPresenter::setSpectraRange(std::size_t minimum,
                                                         std::size_t maximum) {
  m_spectraRange = std::make_pair(minimum, maximum);
}

void IndirectSpectrumSelectionPresenter::updateSpectraList(
    const std::string &spectraList) {
  setSpectraList(spectraList);
  emit spectraChanged(m_spectraList);
}

void IndirectSpectrumSelectionPresenter::updateSpectraRange(
    std::size_t minimum, std::size_t maximum) {
  setSpectraRange(minimum, maximum);
  emit spectraChanged(m_spectraRange);
}

void IndirectSpectrumSelectionPresenter::setBinMask(
    std::size_t index, const std::string &maskString) {
  auto validator = validateMaskBinsString();

  if (validator.isAllInputValid())
    m_binMasks[index] = maskString;
  else
    emit invalidMaskBinsString(validator.generateErrorMessage());
}

void IndirectSpectrumSelectionPresenter::displayBinMask(std::size_t index) {
  m_view->setMaskString(getValueOr(m_binMasks, index, std::string()));
}

UserInputValidator IndirectSpectrumSelectionPresenter::validateSpectraString() {
  UserInputValidator uiv;
  uiv = m_view->validateSpectraString(uiv);
  const auto notInRange =
      outOfRange(m_spectraList, m_minimumSpectrum, m_maximumSpectrum);

  if (!notInRange.empty()) {
    auto message = "Spectra out of range: " + commaSeparatedList(notInRange);
    uiv.addErrorMessage(QString::fromStdString(message));
    m_view->showSpectraErrorLabel();
  }
  return uiv;
}

UserInputValidator
IndirectSpectrumSelectionPresenter::validateMaskBinsString() {
  UserInputValidator uiv;
  return m_view->validateMaskBinsString(uiv);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
