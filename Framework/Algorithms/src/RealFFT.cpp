// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RealFFT.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Exception.h"

#include "MantidKernel/GSL_Helpers.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <sstream>

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include "MantidHistogramData/LinearGenerator.h"

namespace Mantid::Algorithms::RealFFT {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RealFFT)

using namespace Kernel;
using namespace API;

/// Initialisation method. Declares properties to be used in algorithm.
void RealFFT::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::INPUT_WKSP, "", Direction::Input),
      "The name of the input workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "The name of the output workspace. It contains three "
      "spectra: the real, the imaginary parts of the transform and "
      "their modulus.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::WKSP_INDEX, 0, mustBePositive,
                  "The index of the spectrum in the input workspace to transform.");

  std::vector<std::string> fft_dir{"Forward", "Backward"};
  declareProperty(PropertyNames::TRANSFORM, "Forward", std::make_shared<StringListValidator>(fft_dir),
                  R"(The direction of the transform: "Forward" or "Backward".)");
  declareProperty(PropertyNames::IGNORE_X_BINS, false,
                  "Ignores the requirement that X bins be linear and of the same size. "
                  "FFT result will not be valid for the X axis, and should be ignored.");
}

std::map<std::string, std::string> RealFFT::validateInputs() {
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

std::map<std::string, std::string> RealFFT::actuallyValidateInputs(API::Workspace_sptr const &ws) {
  std::map<std::string, std::string> issues;

  // verify a matrix workspace has been passed
  auto inWS = dynamic_cast<API::MatrixWorkspace const *>(ws.get());
  if (!inWS) {
    issues[PropertyNames::INPUT_WKSP] = "RealFFT requires an input matrix workspace";
    return issues;
  }
  std::string transform = getProperty(PropertyNames::TRANSFORM);
  int wi = (transform == "Forward") ? getProperty(PropertyNames::WKSP_INDEX) : 0;
  if (wi >= static_cast<int>(inWS->getNumberHistograms())) {
    issues[PropertyNames::INPUT_WKSP] = "Property WorkspaceIndex is out of range";
    issues[PropertyNames::WKSP_INDEX] = issues[PropertyNames::INPUT_WKSP];
    return issues;
  }
  if (transform == "Backward") {
    if (inWS->getNumberHistograms() < 2)
      issues[PropertyNames::INPUT_WKSP] = "The input workspace must have at least 2 spectra.";
  }

  // Check that the x values are evenly spaced
  bool IgnoreXBins = getProperty(PropertyNames::IGNORE_X_BINS);
  if (!IgnoreXBins) {
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
  return issues;
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if
 */
void RealFFT::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty(PropertyNames::INPUT_WKSP);

  // get the x-spacing
  std::string transform = getProperty(PropertyNames::TRANSFORM);
  int spec = (transform == "Forward") ? getProperty(PropertyNames::WKSP_INDEX) : 0;
  const auto &X = inWS->x(spec);
  double dx = (X.back() - X.front()) / static_cast<double>(X.size() - 1);
  auto ySize = inWS->y(spec).size();

  API::MatrixWorkspace_sptr outWS;

  if (transform == "Forward") {
    // first, transform the data
    auto const &yData = inWS->y(spec);
    std::vector<double> data(yData.cbegin(), yData.cend());
    Kernel::fft::real_ws_uptr workspace = Kernel::fft::make_gsl_real_workspace(ySize);
    Kernel::fft::real_wt_uptr wavetable = Kernel::fft::make_gsl_real_wavetable(ySize);
    gsl_fft_real_transform(data.data(), 1, ySize, wavetable.get(), workspace.get());
    workspace.reset();
    wavetable.reset();

    // unpack the halfcomplex values -- will be full complex, interleaved real/imag
    std::vector<double> unpacked(2 * ySize, 0.0);
    gsl_fft_halfcomplex_unpack(data.data(), unpacked.data(), 1, ySize);

    // second, setup the workspace
    // NOTE: the FT of a real sequence is a half-complex sequence.
    // The "half" part refers to a mirror symmetry z[k] = z*[N-k], NOT to the actual number of complex elements in the
    // transform. There are as many complex elements in the output sequence as real elements in the input sequence.
    // HOWEVER for real data, only the first N/2 points are true measurements; the rest are artifacts
    auto yOutSize = ySize / 2 + 1;
    auto xOutSize = inWS->isHistogramData() ? yOutSize + 1 : yOutSize;

    outWS = WorkspaceFactory::Instance().create(inWS, 3, xOutSize, yOutSize);
    auto tAxis = std::make_unique<API::TextAxis>(3);
    tAxis->setLabel(0, "Real");
    tAxis->setLabel(1, "Imag");
    tAxis->setLabel(2, "Modulus");
    outWS->replaceAxis(1, std::move(tAxis));

    // set the workspace x values
    auto &x = outWS->mutableX(0);
    double df = 1.0 / (dx * static_cast<double>(ySize));
    std::generate(x.begin(), x.end(), HistogramData::LinearGenerator(0, df));
    outWS->setSharedX(1, outWS->sharedX(0));
    outWS->setSharedX(2, outWS->sharedX(0));

    // set the workspace y values
    auto &y1 = outWS->mutableY(0);
    auto &y2 = outWS->mutableY(1);
    auto &y3 = outWS->mutableY(2);
    for (std::size_t i = 0; i < yOutSize; i++) {
      std::size_t const j = i * 2;
      double re = unpacked[j];
      double im = unpacked[j + 1];
      y1[i] = re * dx;                      // real part
      y2[i] = im * dx;                      // imaginary part
      y3[i] = dx * sqrt(re * re + im * im); // modulus
    }
  } else // Backward
  {
    // NOTE for legacy compatibility, the size of the output is computed this way
    std::size_t yOutSize = (ySize - 1) * 2;
    if (inWS->y(1).back() != 0.0) {
      yOutSize++;
    }

    // Setup the half-complex data vector, whose arrangement depends on even/odd of input sequence.

    // If there are N complex elements, there are at most 2 * N real values to store.
    // Half-complex has the symmetry z[k] = z*[N - k], which further reduced storage needs.
    // if N is ODD:  N = 2n + 1, z[1] = z*[N - 1], ..., z[n] = z*[n+1], z[n+1] = z*[n] --> n unique complex values
    // if N is EVEN: N = 2n,     z[1] = z*[N - 1], ..., z[n] = z*[n] is REAL           --> n - 1 unique complex values
    // In both cases, including the pure real elements, the halfcomplex array size is equal to N
    bool even = (yOutSize % 2 == 0);
    std::size_t num_unique_complex = (even ? yOutSize / 2 : (yOutSize - 1) / 2);
    std::vector<double> yhc(yOutSize);

    auto const &yR = inWS->y(0); // real
    auto const &yI = inWS->y(1); // imag

    // Pack the values in a halfcomplex array with GSL format, based on even / odd behavior
    // index | 0         | 1                    |    | n - 1                   | n = num unique complex elements
    // hc    | y[0]      | y[1]       y[2]      | ...| y[2*n - 3]   y[2*n - 2] | y[2*n - 1]   y[2*n]
    // odd   | z[0].real | z[1].real, z[1].imag | ...| z[n-1].real, z[n-1].imag| z[n].real, z[n].imag
    // even  | z[0].real | z[1].real, z[1].imag | ...| z[n-1].real, z[n-1].imag| z[n].real
    yhc[0] = yR[0];
    // starting at 1, interleaved real/imag; odd is an inclusive loop, even exclusive loop with special treatment
    for (std::size_t i = 1; i < num_unique_complex + (even ? 0UL : 1UL); i++) {
      std::size_t const j = 2 * i;
      yhc[j - 1] = yR[i]; // real
      yhc[j] = yI[i];     // imag
    }
    // if even, an unmatched real value at the end
    if (even) {
      yhc[2 * num_unique_complex - 1] = yR[num_unique_complex];
    }

    // Then, inverse transform the data
    Kernel::fft::real_ws_uptr workspace = Kernel::fft::make_gsl_real_workspace(yOutSize);
    Kernel::fft::hc_wt_uptr wavetable = Kernel::fft::make_gsl_hc_wavetable(yOutSize);
    gsl_fft_halfcomplex_inverse(yhc.data(), 1, yOutSize, wavetable.get(), workspace.get());
    wavetable.reset();
    workspace.reset();

    // Finally, setup the output workspace
    std::size_t xOutSize = inWS->isHistogramData() ? yOutSize + 1 : yOutSize;
    double df = 1.0 / (dx * static_cast<double>(yOutSize));
    outWS = WorkspaceFactory::Instance().create(inWS, 1, xOutSize, yOutSize);
    auto tAxis = std::make_unique<API::TextAxis>(1);
    tAxis->setLabel(0, "Real");
    outWS->replaceAxis(1, std::move(tAxis));

    // set the workspace x values
    auto &xData = outWS->mutableX(0);
    std::generate(xData.begin(), xData.end(), HistogramData::LinearGenerator(0, df));

    // set the workspace y values
    auto &yData = outWS->mutableY(0);
    std::move(yhc.begin(), yhc.begin() + yOutSize, yData.begin());
    yData /= df;
  }

  setProperty(PropertyNames::OUTPUT_WKSP, outWS);
}

} // namespace Mantid::Algorithms::RealFFT
