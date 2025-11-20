// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FFTSmooth2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Smoothing.h"

#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid::Algorithms::FFTSmooth {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFTSmooth2)

using namespace Kernel;
using namespace API;

namespace {
enum class FilterType { ZERO, BUTTERWORTH, enum_count };
const std::vector<std::string> filterTypes{"Zeroing", "Butterworth"};
typedef Mantid::Kernel::EnumeratedString<FilterType, &filterTypes> FILTER;
} // namespace

namespace {
typedef std::size_t param_t;
/** FFTParamsProperty
 * The "Params" property of this class should be declared as an ArrayProperty.
 * However, it was originally written to allow many things to serve as an array delimiter.
 * To properly make this an ArrayProperty, while still allowing old behavior, we can use
 * a standatd ArrayProperty and change all of the delimiters to commas.
 */
class FFTParamsProperty : public ArrayProperty<param_t> {
public:
  FFTParamsProperty(std::string const &name, std::vector<param_t> const &defaultValue)
      : ArrayProperty<param_t>(name, defaultValue) {}
  std::string setValue(std::string const &value) override {
    std::string valueCopy(value);
    boost::trim(valueCopy);
    for (char const delim : m_delims) {
      valueCopy = Kernel::Strings::replaceAll(valueCopy, delim, m_sep);
    }
    std::vector<param_t> result;
    toValue(valueCopy, result);
    *this = result;
    return "";
  }

  // Unhide the base class assignment operator
  using PropertyWithValue<std::vector<param_t>>::operator=;

private:
  static char constexpr m_sep = ',';
  static char constexpr m_delims[] = {'\t', ' ', ':', ';'};
};
} // namespace

/// Initialisation method. Declares properties to be used in algorithm.
void FFTSmooth2::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::INPUT_WKSP, "", Direction::Input),
      "The name of the input workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "The name of the output workspace.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::WKSP_INDEX, 0, mustBePositive, "Workspace index for smoothing");

  declareProperty(std::make_unique<Kernel::EnumeratedStringProperty<FilterType, &filterTypes>>(PropertyNames::FILTER),
                  "The type of the applied filter");

  declareProperty(std::make_unique<FFTParamsProperty>(PropertyNames::PARAMS, std::vector<param_t>(2, 2)),
                  "The filter parameters:\n"
                  "For Zeroing, 1 parameter: 'n' - an integer greater than 1 "
                  "meaning that the Fourier coefficients with frequencies "
                  "outside the 1/n of the original range will be set to zero.\n"
                  "For Butterworth, 2 parameters: 'n' and 'order', giving the "
                  "1/n truncation and the smoothing order.\n");

  declareProperty(PropertyNames::IGNORE_X_BINS, false,
                  "Ignores the requirement that X bins be linear and of the same size.\n"
                  "Set this to true if you are using log binning.\n"
                  "The output X axis will be the same as the input either way.");
  declareProperty(PropertyNames::ALL_SPECTRA, false, "Smooth all spectra");
}

std::map<std::string, std::string> FFTSmooth2::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_sptr inWS = getProperty(PropertyNames::INPUT_WKSP);
  auto groupWS = dynamic_cast<API::WorkspaceGroup const *>(inWS.get());
  if (groupWS != nullptr) {
    for (std::size_t idx = 0; idx < groupWS->size(); idx++) {
      auto subissues = this->actuallyValidateInputs(groupWS->getItem(idx));
      for (auto const &[key, value] : subissues) {
        std::string new_key = key + "_" + std::to_string(idx);
        issues[new_key] = value;
      }
    }
  } else {
    issues = this->actuallyValidateInputs(inWS);
  }
  return issues;
}

std::map<std::string, std::string> FFTSmooth2::actuallyValidateInputs(API::Workspace_sptr const &ws) {
  std::map<std::string, std::string> issues;

  // verify a matrix workspace has been passed
  auto inWS = dynamic_cast<API::MatrixWorkspace const *>(ws.get());
  if (!inWS) {
    issues[PropertyNames::INPUT_WKSP] = "FFTSmooth requires an input matrix workspace";
    return issues;
  }
  // verify the spectrum workspace index
  int wi = getProperty(PropertyNames::WKSP_INDEX);
  if (wi >= static_cast<int>(inWS->getNumberHistograms())) {
    issues[PropertyNames::INPUT_WKSP] = "Property WorkspaceIndex is out of range";
    issues[PropertyNames::WKSP_INDEX] = issues[PropertyNames::INPUT_WKSP];
    return issues;
  }

  // Check that the x values are evenly spaced
  bool ignoreXBins = getProperty(PropertyNames::IGNORE_X_BINS);
  if (!ignoreXBins) {
    const auto &X = inWS->x(wi);
    double dx = (X.back() - X.front()) / static_cast<double>(X.size() - 1);
    for (size_t i = 0; i < X.size() - 2; i++) {
      if (std::abs(dx - X[i + 1] + X[i]) / dx > 1e-7) {
        issues[PropertyNames::INPUT_WKSP] = "X axis must be linear (all bins have same width). This can be ignored if "
                                            "IgnoreXBins is set to true.";
        issues[PropertyNames::IGNORE_X_BINS] = issues[PropertyNames::INPUT_WKSP];
        break;
      }
    }
  }

  // Check that the parameters are valid
  std::vector<param_t> params = getProperty(PropertyNames::PARAMS);
  std::string const trunc_err = "Cutoff parameter must be an integer > 1. ";
  std::string const order_err = "Butterworth filter order must be an integer >= 1. ";
  std::string err_msg;
  if (params.size() > 0 && params[0] <= 1) {
    err_msg += trunc_err;
  }
  if (params.size() > 1 && params[1] < 1) {
    err_msg += order_err;
  }
  if (params.size() > 2) {
    err_msg += "Too many parameters passed";
  }
  if (!err_msg.empty()) {
    issues[PropertyNames::PARAMS] = std::move(err_msg);
  }
  return issues;
}

/** Executes the algorithm
 */
void FFTSmooth2::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty(PropertyNames::INPUT_WKSP);

  // retrieve parameters
  std::size_t n = 2, order = 2;
  std::vector<std::size_t> params = getProperty(PropertyNames::PARAMS);
  if (params.size() == 1) {
    n = params[0];
  } else if (params.size() == 2) {
    n = params[0];
    order = params[1];
  }

  std::size_t dn = inWS->y(0).size();

  // set smoothing method based on user setting
  FILTER type = getPropertyValue(PropertyNames::FILTER);
  std::function<std::vector<double>(std::vector<double> const &)> smoothMethod;
  switch (type) {
  case FilterType::ZERO: {
    // NOTE this algorithm used a cutoff *period*, whereas fftSmooth
    // uses a cutoff number (a quantum number).  The below adjusted cutoff is precisely
    // the value needed for fftSmooth to have the same result as the prior behavior
    // This takes into account BOTH the symmetrization op below, AND the halfcomplex packing
    // |  symm_size = 2 * dn
    // |  halfcomplex size of symm = symm_size / 2 = dn
    // so correct size to use is dn
    unsigned adjusted_cutoff = (n > dn ? 1 : static_cast<unsigned>((dn + 1) / n));
    smoothMethod = [adjusted_cutoff](std::vector<double> const &y) {
      return Kernel::Smoothing::fftSmooth(y, adjusted_cutoff);
    };
    break;
  }
  case FilterType::BUTTERWORTH: {
    // see note above about adjusted cutoff
    unsigned adjusted_cutoff = (n > dn ? 1 : static_cast<unsigned>((dn + 1) / n));
    smoothMethod = [adjusted_cutoff, order](std::vector<double> const &y) {
      return Kernel::Smoothing::fftButterworthSmooth(y, adjusted_cutoff, static_cast<unsigned int>(order));
    };
    break;
  }
  default: {
    smoothMethod = [](std::vector<double> const &y) { return y; };
  }
  }

  // First spectrum in input
  int s0 = getProperty(PropertyNames::WKSP_INDEX);
  // By default only do one
  std::size_t send = s0 + 1;
  if (getProperty(PropertyNames::ALL_SPECTRA)) { // Except if AllSpectra
    s0 = 0;
    send = inWS->getNumberHistograms();
  }
  // Create output
  API::MatrixWorkspace_sptr outWS =
      API::WorkspaceFactory::Instance().create(inWS, send - s0, inWS->x(0).size(), inWS->y(0).size());

  Progress progress(this, 0.0, 1.0, 4 * (send - s0));

  for (std::size_t spec = s0; spec < send; spec++) {
    progress.report();

    // Symmetrize the input vector.
    // The below pictograph illustrates the transformation from original to "symmetrized"
    //            o            :     oo             o
    //       oo  o|            :      |o  oo   oo  o|
    //      o  oo |          ---->    | oo  o o  oo |
    //     o      |            :      |      o      |
    //     ^i=0   ^i=dn-1      :      ^i=1   ^i=dn  ^i=2*dn-1
    // NOTE it's unknown why this operation was originally added.
    // It is necessary to retain this operation to support legacy behavior.
    std::vector<double> symY(2 * dn);
    for (std::size_t i = 0; i < dn; i++) {
      symY[dn + i] = inWS->y(spec)[i];
      symY[dn - i] = inWS->y(spec)[i];
    }
    symY.front() = inWS->y(spec).back();

    // Apply the filter
    progress.report("Applying Filter");
    std::vector<double> tmpY = smoothMethod(symY);

    // assign
    if (getProperty(PropertyNames::ALL_SPECTRA)) {
      outWS->setSharedX(spec, inWS->sharedX(spec));
      outWS->mutableY(spec).assign(tmpY.cbegin() + dn, tmpY.cend());
    } else {
      outWS->setSharedX(0, inWS->sharedX(spec));
      outWS->mutableY(0).assign(tmpY.cbegin() + dn, tmpY.cend());
    }
  }

  setProperty(PropertyNames::OUTPUT_WKSP, outWS);
}
} // namespace Mantid::Algorithms::FFTSmooth
