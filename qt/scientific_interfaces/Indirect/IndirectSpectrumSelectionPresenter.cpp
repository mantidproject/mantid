#include "IndirectSpectrumSelectionPresenter.h"

#include "MantidKernel/ArrayProperty.h"

#include <algorithm>
#include <iterator>
#include <sstream>

#include <boost/numeric/conversion/cast.hpp>

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

struct SetViewSpectra : boost::static_visitor<> {
  SetViewSpectra(IndirectSpectrumSelectionView *view) : m_view(view) {}

  void operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    m_view->displaySpectra(spectra.first, spectra.second);
  }

  void operator()(const std::string &spectra) const {
    m_view->displaySpectra(spectra);
  }

private:
  IndirectSpectrumSelectionView *m_view;
};

}; // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

namespace Regexes {
const std::string EMPTY = "^$";
const std::string SPACE = "(\\s)*";
const std::string COMMA = SPACE + "," + SPACE;
const std::string MINUS = SPACE + "\\-" + SPACE;
const std::string NUMBER = "(0|[1-9][0-9]*)";
const std::string RANGE = "(" + NUMBER + MINUS + NUMBER + ")";
const std::string NUMBER_OR_RANGE = "(" + RANGE + "|" + NUMBER + ")";
const std::string SPECTRA_LIST =
    "(" + NUMBER_OR_RANGE + "(" + COMMA + NUMBER_OR_RANGE + ")*)";
const std::string MASK_LIST =
    "(" + RANGE + "(" + COMMA + RANGE + ")*" + ")|" + EMPTY;
} // namespace Regexes

IndirectSpectrumSelectionPresenter::IndirectSpectrumSelectionPresenter(
    IndirectFittingModel *model, IndirectSpectrumSelectionView *view)
    : QObject(nullptr), m_model(model), m_view(view) {
  connect(m_view.get(), SIGNAL(selectedSpectraChanged(const std::string &)),
          this, SLOT(updateSpectraList(const std::string &)));
  connect(m_view.get(),
          SIGNAL(selectedSpectraChanged(std::size_t, std::size_t)), this,
          SLOT(updateSpectraRange(std::size_t, std::size_t)));

  connect(m_view.get(), SIGNAL(maskChanged(std::size_t, const std::string &)),
          this, SLOT(setBinMask(std::size_t, const std::string &)));
  connect(m_view.get(), SIGNAL(maskChanged(std::size_t, const std::string &)),
          this, SLOT(setMaskSpectraList(std::size_t, const std::string &)));
  connect(m_view.get(), SIGNAL(maskSpectrumChanged(std::size_t)), this,
          SLOT(displayBinMask(std::size_t index)));

  connect(m_view.get(), SIGNAL(maskChanged(std::size_t, const std::string &)),
          this, SIGNAL(maskChanged(std::size_t, const std::string &)));

  m_view->setSpectraRegex(Regexes::SPECTRA_LIST);
  m_view->setMaskBinsRegex(Regexes::MASK_LIST);

  if (nullptr == model->getWorkspace(0))
    view->setEnabled(false);
}

IndirectSpectrumSelectionPresenter::~IndirectSpectrumSelectionPresenter() {}

void IndirectSpectrumSelectionPresenter::setActiveModelIndex(
    std::size_t index) {
  const auto workspace = m_model->getWorkspace(index);
  setSpectraRange(0, workspace->getNumberHistograms());
  boost::apply_visitor(SetViewSpectra(m_view.get()), m_model->getSpectra(index));
  m_activeIndex = index;
  m_view->setEnabled(workspace ? true : false);
}

void IndirectSpectrumSelectionPresenter::setSpectraRange(std::size_t minimum,
                                                         std::size_t maximum) {
  int minimumInt = boost::numeric_cast<int>(minimum);
  int maximumInt = boost::numeric_cast<int>(maximum);
  m_view->setSpectraRange(minimumInt, maximumInt);
  m_view->setMaskSpectraRange(minimumInt, maximumInt);
}

void IndirectSpectrumSelectionPresenter::setModelSpectra(
    const Spectra &spectra) {
  try {
    m_model->setSpectra(m_view->spectraString(), m_activeIndex);
    m_spectraError.clear();
  } catch (const std::runtime_error &ex) {
    m_spectraError = ex.what();
    m_view->showSpectraErrorLabel();
  }
}

void IndirectSpectrumSelectionPresenter::updateSpectraList(
    const std::string &spectraList) {
  setModelSpectra(spectraList);
  emit spectraChanged(Spectra(spectraList));
}

void IndirectSpectrumSelectionPresenter::updateSpectraRange(
    std::size_t minimum, std::size_t maximum) {
  setModelSpectra(std::make_pair(minimum, maximum));
  emit spectraChanged(Spectra(std::make_pair(minimum, maximum)));
}

void IndirectSpectrumSelectionPresenter::setMaskSpectraList(
    std::size_t, const std::string &maskString) {
  m_view->setMaskSpectraList(vectorFromString<std::size_t>(maskString));
}

void IndirectSpectrumSelectionPresenter::setBinMask(
    std::size_t index, const std::string &maskString) {
  auto validator = validateMaskBinsString();

  if (validator.isAllInputValid())
    m_model->setExcludeRegion(maskString, m_activeIndex, index);
  else
    emit invalidMaskBinsString(validator.generateErrorMessage());
}

void IndirectSpectrumSelectionPresenter::displayBinMask(std::size_t index) {
  m_view->setMaskString(m_model->getExcludeRegion(index));
}

UserInputValidator &
IndirectSpectrumSelectionPresenter::validate(UserInputValidator &validator) {
  auto uiv = validateSpectraString();
  return m_view->validateMaskBinsString(validator);
}

UserInputValidator IndirectSpectrumSelectionPresenter::validateSpectraString() {
  UserInputValidator uiv;
  uiv = m_view->validateSpectraString(uiv);

  if (!m_spectraError.empty())
    uiv.addErrorMessage(QString::fromStdString(m_spectraError));
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
