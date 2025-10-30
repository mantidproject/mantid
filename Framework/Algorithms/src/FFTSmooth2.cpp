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
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Smoothing.h"

#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFTSmooth2)

using namespace Kernel;
using namespace API;

namespace {
enum class FilterType { ZERO, BUTTERWORTH, enum_count };
const std::vector<std::string> filterTypes{"Zeroing", "Butterworth"};
typedef Mantid::Kernel::EnumeratedString<FilterType, &filterTypes> FILTER;
} // namespace

/// Initialisation method. Declares properties to be used in algorithm.
void FFTSmooth2::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The name of the input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBePositive, "Workspace index for smoothing");

  declareProperty(std::make_unique<Mantid::Kernel::EnumeratedStringProperty<FilterType, &filterTypes>>("Filter"),
                  "The type of the applied filter");

  declareProperty("Params", "",
                  "The filter parameters:\n"
                  "For Zeroing, 1 parameter: 'n' - an integer greater than 1 "
                  "meaning that the Fourier coefficients with frequencies "
                  "outside the 1/n of the original range will be set to zero.\n"
                  "For Butterworth, 2 parameters: 'n' and 'order', giving the "
                  "1/n truncation and the smoothing order.\n");

  declareProperty("IgnoreXBins", false,
                  "Ignores the requirement that X bins be linear and of the same size.\n"
                  "Set this to true if you are using log binning.\n"
                  "The output X axis will be the same as the input either way.");
  declareProperty("AllSpectra", false, "Smooth all spectra");
}

std::map<std::string, std::string> FFTSmooth2::validateInputs() {
  std::map<std::string, std::string> issues;

  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  // verify the spectrum workspace index
  int wi = getProperty("WorkspaceIndex");
  if (wi >= static_cast<int>(inWS->getNumberHistograms())) {
    issues["WorkspaceIndex"] = "Property WorkspaceIndex is out of range";
    return issues;
  }

  // Check that the x values are evenly spaced
  bool IgnoreXBins = getProperty("IgnoreXBins");
  if (!IgnoreXBins) {
    const auto &X = inWS->x(wi);
    double dx = (X.back() - X.front()) / static_cast<double>(X.size() - 1);
    for (size_t i = 0; i < X.size() - 2; i++) {
      if (std::abs(dx - X[i + 1] + X[i]) / dx > 1e-7) {
        issues["InputWorkspace"] = "X axis must be linear (all bins have same "
                                   "width). This can be ignored if "
                                   "IgnoreXBins is set to true.";
        break;
      }
    }
  }
  return issues;
}

/** Executes the algorithm
 */
void FFTSmooth2::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  // retrieve parameters
  unsigned n = 2, order = 2;
  std::string string_params = getProperty("Params");
  std::vector<std::string> params;
  boost::split(params, string_params, boost::algorithm::detail::is_any_ofF<char>(" ,:;\t"));
  if (params.size() == 1) {
    std::string param0 = params.at(0);
    n = std::stoi(param0);
    order = 2;
  } else if (params.size() == 2) {
    std::string param0 = params.at(0);
    std::string param1 = params.at(1);
    n = std::stoi(param0);
    order = std::stoi(param1);
  }
  if (n <= 1)
    throw std::invalid_argument("Truncation parameter must be an integer > 1");
  if (order < 1)
    throw std::invalid_argument("Butterworth filter order must be an integer >= 1");

  std::size_t dn = inWS->y(0).size();

  // set smoothing method based on user setting
  FILTER type = getPropertyValue("Filter");
  std::function<std::vector<double>(std::vector<double> const &)> smoothMethod;
  switch (type) {
  case FilterType::ZERO: {
    // NOTE this algorithm used a cutoff *frequency*, whereas fftSmooth
    // uses a cutoff number (a quantum number).  The below ny is precisely
    // the value needed for fftSmooth to have the same result as the prior behavior
    // this takes into account BOTH the symmetrization op below, AND the halfcomplex packing
    // |  symm_size = 2 * dn
    // |  halfcomplex size of symm = symm_size / 2 = dn
    // so correct size to use is dn
    unsigned adjusted_cutoff = (n > dn ? 1 : static_cast<unsigned>(dn / n));
    smoothMethod = [adjusted_cutoff](std::vector<double> const &y) {
      return Kernel::Smoothing::fftSmooth(y, adjusted_cutoff);
    };
    break;
  }
  case FilterType::BUTTERWORTH: {
    unsigned adjusted_cutoff = (n > dn ? 1 : static_cast<unsigned>(dn / n));
    smoothMethod = [adjusted_cutoff, order](std::vector<double> const &y) {
      return Kernel::Smoothing::fftButterworthSmooth(y, adjusted_cutoff, order);
    };
    break;
  }
  default: {
    smoothMethod = [](std::vector<double> const &y) { return y; };
  }
  }

  // First spectrum in input
  int s0 = getProperty("WorkspaceIndex");
  // By default only do one
  int send = s0 + 1;
  if (getProperty("AllSpectra")) { // Except if AllSpectra
    s0 = 0;
    send = static_cast<int>(inWS->getNumberHistograms());
  }
  // Create output
  API::MatrixWorkspace_sptr outWS =
      API::WorkspaceFactory::Instance().create(inWS, send - s0, inWS->x(0).size(), inWS->y(0).size());

  // Symmetrize the input spectrum
  Progress progress(this, 0.0, 1.0, 4 * (send - s0));

  for (int spec = s0; spec < send; spec++) {
    progress.report();

    // suppose original is graph like
    //      ....
    //     .     .
    //   .        .
    //  .          .
    // .
    // ^i=0
    // then the symmetrized looks like
    //     ....         ....
    //   .     .       .     .
    //  .        .   .        .
    // .          . .          .
    //             .
    // ^i=0        ^i = dn
    std::vector<double> symY(2 * dn);
    for (std::size_t i = 0; i < dn; i++) {
      symY[dn + i] = inWS->y(spec)[i];
      symY[dn - i] = inWS->y(spec)[i];
    }
    symY.front() = inWS->y(spec).back();

    // Apply the filter
    progress.report("Applying Filter");
    std::vector<double> tmpY = smoothMethod(symY);

    // FIXME: The intent of the following line is not clear.
    // std::floor or std::ceil should probably be used.
    dn = tmpY.size() / 2;

    // assign
    if (getProperty("AllSpectra")) {
      outWS->setSharedX(spec, inWS->sharedX(spec));
      outWS->mutableY(spec).assign(tmpY.cbegin() + dn, tmpY.cend());
    } else {
      outWS->setSharedX(0, inWS->sharedX(spec));
      outWS->mutableY(0).assign(tmpY.cbegin() + dn, tmpY.cend());
    }
  }

  setProperty("OutputWorkspace", outWS);
}
} // namespace Mantid::Algorithms
