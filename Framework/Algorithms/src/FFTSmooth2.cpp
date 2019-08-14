// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FFTSmooth2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"

#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFTSmooth2)

using namespace Kernel;
using namespace API;

/// Initialisation method. Declares properties to be used in algorithm.
void FFTSmooth2::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBePositive,
                  "Workspace index for smoothing");

  std::vector<std::string> type{"Zeroing", "Butterworth"};
  declareProperty("Filter", "Zeroing",
                  boost::make_shared<StringListValidator>(type),
                  "The type of the applied filter");
  declareProperty("Params", "",
                  "The filter parameters:\n"
                  "For Zeroing, 1 parameter: 'n' - an integer greater than 1 "
                  "meaning that the Fourier coefficients with frequencies "
                  "outside the 1/n of the original range will be set to zero.\n"
                  "For Butterworth, 2 parameters: 'n' and 'order', giving the "
                  "1/n truncation and the smoothing order.\n");

  declareProperty(
      "IgnoreXBins", false,
      "Ignores the requirement that X bins be linear and of the same size.\n"
      "Set this to true if you are using log binning.\n"
      "The output X axis will be the same as the input either way.");
  declareProperty("AllSpectra", false, "Smooth all spectra");
}

/** Executes the algorithm
 */
void FFTSmooth2::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  /// Will we Allow Any X Bins?
  bool ignoreXBins = getProperty("IgnoreXBins");

  // First spectrum in input
  int s0 = getProperty("WorkspaceIndex");
  // By default only do one
  int send = s0 + 1;
  if (getProperty("AllSpectra")) { // Except if AllSpectra
    s0 = 0;
    send = static_cast<int>(inWS->getNumberHistograms());
  }
  // Create output
  API::MatrixWorkspace_sptr outWS = API::WorkspaceFactory::Instance().create(
      inWS, send - s0, inWS->x(0).size(), inWS->y(0).size());

  // Symmetrize the input spectrum
  auto dn = static_cast<int>(inWS->y(0).size());
  API::MatrixWorkspace_sptr symmWS = API::WorkspaceFactory::Instance().create(
      "Workspace2D", 1, inWS->x(0).size() + dn, inWS->y(0).size() + dn);

  Progress progress(this, 0.0, 1.0, 4 * (send - s0));

  for (int spec = s0; spec < send; spec++) {
    // Save the starting x value so it can be restored after all transforms.
    double x0 = inWS->x(spec)[0];

    double dx = (inWS->x(spec).back() - inWS->x(spec).front()) /
                (static_cast<double>(inWS->x(spec).size()) - 1.0);

    progress.report();

    auto &symX = symmWS->mutableX(0);
    auto &symY = symmWS->mutableY(0);

    for (int i = 0; i < dn; i++) {
      symX[dn + i] = inWS->x(spec)[i];
      symY[dn + i] = inWS->y(spec)[i];

      symX[dn - i] = x0 - dx * i;
      symY[dn - i] = inWS->y(spec)[i];
    }
    symY.front() = inWS->y(spec).back();
    symX.front() = x0 - dx * dn;
    if (inWS->isHistogramData())
      symX.back() = inWS->x(spec).back();

    // setProperty("OutputWorkspace",symmWS); return;

    progress.report("Calculating FFT");
    // Forward Fourier transform
    IAlgorithm_sptr fft = createChildAlgorithm("RealFFT", 0, 0.5);
    fft->setProperty("InputWorkspace", symmWS);
    fft->setProperty("WorkspaceIndex", 0);
    fft->setProperty("IgnoreXBins", ignoreXBins);
    try {
      fft->execute();
    } catch (...) {
      g_log.error("Error in direct FFT algorithm");
      throw;
    }

    API::MatrixWorkspace_sptr unfilteredWS =
        fft->getProperty("OutputWorkspace");
    API::MatrixWorkspace_sptr filteredWS;

    // Apply the filter
    std::string type = getProperty("Filter");

    if (type == "Zeroing") {
      std::string sn = getProperty("Params");
      int n;
      if (sn.empty())
        n = 2;
      else
        n = std::stoi(sn);
      if (n <= 1)
        throw std::invalid_argument(
            "Truncation parameter must be an integer > 1");

      progress.report("Zero Filter");

      zero(n, unfilteredWS, filteredWS);
    } else if (type == "Butterworth") {
      int n, order;

      std::string string_params = getProperty("Params");
      std::vector<std::string> params;
      boost::split(params, string_params,
                   boost::algorithm::detail::is_any_ofF<char>(" ,:;\t"));
      if (params.size() != 2) {
        n = 2;
        order = 2;
      } else {
        std::string param0 = params.at(0);
        std::string param1 = params.at(1);
        n = std::stoi(param0);
        order = std::stoi(param1);
      }
      if (n <= 1)
        throw std::invalid_argument(
            "Truncation parameter must be an integer > 1");
      if (order < 1)
        throw std::invalid_argument(
            "Butterworth filter order must be an integer >= 1");

      progress.report("ButterWorth Filter");
      Butterworth(n, order, unfilteredWS, filteredWS);
    }

    progress.report("Backward Transformation");
    // Backward transform
    fft = createChildAlgorithm("RealFFT", 0.5, 1.);
    fft->setProperty("InputWorkspace", filteredWS);
    fft->setProperty("Transform", "Backward");
    fft->setProperty("IgnoreXBins", ignoreXBins);
    try {
      fft->execute();
    } catch (...) {
      g_log.error("Error in inverse FFT algorithm");
      throw;
    }
    API::MatrixWorkspace_sptr tmpWS = fft->getProperty("OutputWorkspace");

    // FIXME: The intent of the following line is not clear. std::floor or
    // std::ceil should
    // probably be used.
    dn = static_cast<int>(tmpWS->blocksize()) / 2;

    if (getProperty("AllSpectra")) {
      outWS->setSharedX(spec, inWS->sharedX(spec));
      outWS->mutableY(spec).assign(tmpWS->y(0).cbegin() + dn,
                                   tmpWS->y(0).cend());
    } else {
      outWS->setSharedX(0, inWS->sharedX(spec));
      outWS->mutableY(0).assign(tmpWS->y(0).cbegin() + dn, tmpWS->y(0).cend());
    }
  }

  setProperty("OutputWorkspace", outWS);
}

/** Smoothing by zeroing.
 *  @param n :: The order of truncation
 *  @param unfilteredWS :: workspace for storing the unfiltered Fourier
 * transform of the input spectrum
 *  @param filteredWS :: workspace for storing the filtered spectrum
 */
void FFTSmooth2::zero(int n, API::MatrixWorkspace_sptr &unfilteredWS,
                      API::MatrixWorkspace_sptr &filteredWS) {
  auto mx = static_cast<int>(unfilteredWS->x(0).size());
  auto my = static_cast<int>(unfilteredWS->y(0).size());
  int ny = my / n;

  if (ny == 0)
    ny = 1;

  filteredWS =
      API::WorkspaceFactory::Instance().create(unfilteredWS, 2, mx, my);

  filteredWS->setSharedX(0, unfilteredWS->sharedX(0));
  filteredWS->setSharedX(1, unfilteredWS->sharedX(0));

  std::copy(unfilteredWS->y(0).cbegin(), unfilteredWS->y(0).begin() + ny,
            filteredWS->mutableY(0).begin());

  std::copy(unfilteredWS->y(1).cbegin(), unfilteredWS->y(1).begin() + ny,
            filteredWS->mutableY(1).begin());
}

/** Smoothing using Butterworth filter.
 *  @param n ::     The cutoff frequency control parameter.
 *               Cutoff frequency = my/n where my is the
 *               number of sample points in the data.
 *               As with the "Zeroing" case, the cutoff
 *               frequency is truncated to an integer value
 *               and set to 1 if the truncated value was zero.
 *  @param order :: The order of the Butterworth filter, 1, 2, etc.
 *               This must be a positive integer.
 *  @param unfilteredWS :: workspace for storing the unfiltered Fourier
 * transform of the input spectrum
 *  @param filteredWS :: workspace for storing the filtered spectrum
 */
void FFTSmooth2::Butterworth(int n, int order,
                             API::MatrixWorkspace_sptr &unfilteredWS,
                             API::MatrixWorkspace_sptr &filteredWS) {
  auto mx = static_cast<int>(unfilteredWS->x(0).size());
  auto my = static_cast<int>(unfilteredWS->y(0).size());
  int ny = my / n;

  if (ny == 0)
    ny = 1;

  filteredWS =
      API::WorkspaceFactory::Instance().create(unfilteredWS, 2, mx, my);

  filteredWS->setSharedX(0, unfilteredWS->sharedX(0));
  filteredWS->setSharedX(1, unfilteredWS->sharedX(0));

  auto &Yr = unfilteredWS->y(0);
  auto &Yi = unfilteredWS->y(1);
  auto &yr = filteredWS->mutableY(0);
  auto &yi = filteredWS->mutableY(1);

  double cutoff = ny;

  for (int i = 0; i < my; i++) {
    double scale =
        1.0 / (1.0 + pow(static_cast<double>(i) / cutoff, 2 * order));
    yr[i] = scale * Yr[i];
    yi[i] = scale * Yi[i];
  }
}

} // namespace Algorithms
} // namespace Mantid
