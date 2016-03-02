//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFTSmooth2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/detail/classification.hpp>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFTSmooth2)

using namespace Kernel;
using namespace API;

/// Initialisation method. Declares properties to be used in algorithm.
void FFTSmooth2::init() {
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the input workspace.");
  declareProperty(make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace.");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBePositive,
                  "Spectrum index for smoothing");

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
      inWS, send - s0, inWS->readX(0).size(), inWS->readY(0).size());

  // Symmetrize the input spectrum
  int dn = static_cast<int>(inWS->readY(0).size());
  API::MatrixWorkspace_sptr symmWS = API::WorkspaceFactory::Instance().create(
      "Workspace2D", 1, inWS->readX(0).size() + dn, inWS->readY(0).size() + dn);

  Progress progress(this, 0, 1, 4 * (send - s0));

  for (int spec = s0; spec < send; spec++) {
    // Save the starting x value so it can be restored after all transforms.
    double x0 = inWS->readX(spec)[0];

    double dx = (inWS->readX(spec).back() - inWS->readX(spec).front()) /
                (static_cast<double>(inWS->readX(spec).size()) - 1.0);

    progress.report();
    for (int i = 0; i < dn; i++) {
      symmWS->dataX(0)[dn + i] = inWS->readX(spec)[i];
      symmWS->dataY(0)[dn + i] = inWS->readY(spec)[i];

      symmWS->dataX(0)[dn - i] = x0 - dx * i;
      symmWS->dataY(0)[dn - i] = inWS->readY(spec)[i];
    }
    symmWS->dataY(0).front() = inWS->readY(spec).back();
    symmWS->dataX(0).front() = x0 - dx * dn;
    if (inWS->isHistogramData())
      symmWS->dataX(0).back() = inWS->readX(spec).back();

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
        n = atoi(sn.c_str());
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
        n = atoi(param0.c_str());
        order = atoi(param1.c_str());
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
      outWS->dataX(spec)
          .assign(inWS->readX(spec).begin(), inWS->readX(spec).end());
      outWS->dataY(spec)
          .assign(tmpWS->readY(0).begin() + dn, tmpWS->readY(0).end());
    } else {
      outWS->dataX(0)
          .assign(inWS->readX(spec).begin(), inWS->readX(spec).end());
      outWS->dataY(0)
          .assign(tmpWS->readY(0).begin() + dn, tmpWS->readY(0).end());
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
  int mx = static_cast<int>(unfilteredWS->readX(0).size());
  int my = static_cast<int>(unfilteredWS->readY(0).size());
  int ny = my / n;

  if (ny == 0)
    ny = 1;

  filteredWS =
      API::WorkspaceFactory::Instance().create(unfilteredWS, 2, mx, my);

  const Mantid::MantidVec &Yr = unfilteredWS->readY(0);
  const Mantid::MantidVec &Yi = unfilteredWS->readY(1);
  const Mantid::MantidVec &X = unfilteredWS->readX(0);

  Mantid::MantidVec &yr = filteredWS->dataY(0);
  Mantid::MantidVec &yi = filteredWS->dataY(1);
  Mantid::MantidVec &xr = filteredWS->dataX(0);
  Mantid::MantidVec &xi = filteredWS->dataX(1);

  xr.assign(X.begin(), X.end());
  xi.assign(X.begin(), X.end());
  yr.assign(Yr.size(), 0);
  yi.assign(Yr.size(), 0);

  for (int i = 0; i < ny; i++) {
    yr[i] = Yr[i];
    yi[i] = Yi[i];
  }
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
  int mx = static_cast<int>(unfilteredWS->readX(0).size());
  int my = static_cast<int>(unfilteredWS->readY(0).size());
  int ny = my / n;

  if (ny == 0)
    ny = 1;

  filteredWS =
      API::WorkspaceFactory::Instance().create(unfilteredWS, 2, mx, my);

  const Mantid::MantidVec &Yr = unfilteredWS->readY(0);
  const Mantid::MantidVec &Yi = unfilteredWS->readY(1);
  const Mantid::MantidVec &X = unfilteredWS->readX(0);

  Mantid::MantidVec &yr = filteredWS->dataY(0);
  Mantid::MantidVec &yi = filteredWS->dataY(1);
  Mantid::MantidVec &xr = filteredWS->dataX(0);
  Mantid::MantidVec &xi = filteredWS->dataX(1);

  xr.assign(X.begin(), X.end());
  xi.assign(X.begin(), X.end());
  yr.assign(Yr.size(), 0);
  yi.assign(Yr.size(), 0);

  double cutoff = ny;

  for (int i = 0; i < my; i++) {
    double scale = 1.0 / (1.0 + pow(i / cutoff, 2 * order));
    yr[i] = scale * Yr[i];
    yi[i] = scale * Yi[i];
  }
}

} // namespace Algorithm
} // namespace Mantid
