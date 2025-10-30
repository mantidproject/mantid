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
#include "MantidKernel/Exception.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_fft_real.h>

#define REAL(z, i) ((z)[2 * (i)])
#define IMAG(z, i) ((z)[2 * (i) + 1])

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <sstream>

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include "MantidHistogramData/LinearGenerator.h"

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RealFFT)

using namespace Kernel;
using namespace API;

/// Initialisation method. Declares properties to be used in algorithm.
void RealFFT::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The name of the input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the output workspace. It contains three "
                  "spectra: the real, the imaginary parts of the transform and "
                  "their modulus.");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBePositive,
                  "The index of the spectrum in the input workspace to transform.");

  std::vector<std::string> fft_dir{"Forward", "Backward"};
  declareProperty("Transform", "Forward", std::make_shared<StringListValidator>(fft_dir),
                  R"(The direction of the transform: "Forward" or "Backward".)");
  declareProperty("IgnoreXBins", false,
                  "Ignores the requirement that X bins be linear and of the same size. "
                  "FFT result will not be valid for the X axis, and should be ignored.");
}

std::map<std::string, std::string> RealFFT::validateInputs() {
  std::map<std::string, std::string> issues;

  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  // verify the spectrum workspace index
  std::string transform = getProperty("Transform");
  int wi = (transform == "Forward") ? getProperty("WorkspaceIndex") : 0;
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
 *
 *  @throw runtime_error Thrown if
 */
void RealFFT::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

  // get the x-spacing
  std::string transform = getProperty("Transform");
  int spec = (transform == "Forward") ? getProperty("WorkspaceIndex") : 0;
  const auto &X = inWS->x(spec);
  double dx = (X.back() - X.front()) / static_cast<double>(X.size() - 1);

  auto ySize = inWS->blocksize();
  double df = 1.0 / (dx * static_cast<double>(ySize));

  API::MatrixWorkspace_sptr outWS;

  if (transform == "Forward") {
    // first, transform the data
    std::vector<double> data(2 * ySize, 0);
    auto &yData = inWS->mutableY(spec);
    std::copy(yData.cbegin(), yData.cend(), data.begin());
    gsl_fft_real_workspace *workspace = gsl_fft_real_workspace_alloc(ySize);
    gsl_fft_real_wavetable *wavetable = gsl_fft_real_wavetable_alloc(ySize);
    gsl_fft_real_transform(data.data(), 1, ySize, wavetable, workspace);
    gsl_fft_real_wavetable_free(wavetable);
    gsl_fft_real_workspace_free(workspace);

    // second, setup the workspace
    auto yOutSize = ySize / 2 + 1;
    auto xOutSize = inWS->isHistogramData() ? yOutSize + 1 : yOutSize;
    bool odd = ySize % 2 != 0;

    outWS = WorkspaceFactory::Instance().create(inWS, 3, xOutSize, yOutSize);
    auto tAxis = std::make_unique<API::TextAxis>(3);
    tAxis->setLabel(0, "Real");
    tAxis->setLabel(1, "Imag");
    tAxis->setLabel(2, "Modulus");
    outWS->replaceAxis(1, std::move(tAxis));

    auto &x = outWS->mutableX(0);
    auto &y1 = outWS->mutableY(0);
    auto &y2 = outWS->mutableY(1);
    auto &y3 = outWS->mutableY(2);
    for (std::size_t i = 0; i < yOutSize; i++) {
      std::size_t j = i * 2;
      x[i] = df * static_cast<double>(i);
      double re = i != 0 ? data[j - 1] : data[0];
      double im = (i != 0 && (odd || i != yOutSize - 1)) ? data[j] : 0;
      y1[i] = re * dx;                      // real part
      y2[i] = im * dx;                      // imaginary part
      y3[i] = dx * sqrt(re * re + im * im); // modulus
    }
    if (inWS->isHistogramData()) {
      outWS->mutableX(0)[yOutSize] = outWS->mutableX(0)[yOutSize - 1] + df;
    }
    outWS->setSharedX(1, outWS->sharedX(0));
    outWS->setSharedX(2, outWS->sharedX(0));
  } else // Backward
  {
    if (inWS->getNumberHistograms() < 2)
      throw std::runtime_error("The input workspace must have at least 2 spectra.");

    // first, setup the data vector
    std::size_t yOutSize = (ySize - 1) * 2;
    if (inWS->y(1).back() != 0.0)
      yOutSize++;
    bool odd = yOutSize % 2 != 0;

    auto &y0 = inWS->mutableY(0);
    auto &y1 = inWS->mutableY(1);
    std::vector<double> yhc(yOutSize);
    for (std::size_t i = 0; i < ySize; i++) {
      if (i != 0) {
        std::size_t j = i * 2;
        yhc[j - 1] = y0[i];
        if (odd || i != ySize - 1) {
          yhc[j] = y1[i];
        }
      } else {
        yhc[0] = y0[0];
      }
    }

    // then, inverse transform the data
    gsl_fft_real_workspace *workspace = gsl_fft_real_workspace_alloc(yOutSize);
    gsl_fft_halfcomplex_wavetable *wavetable = gsl_fft_halfcomplex_wavetable_alloc(yOutSize);
    // &(yData[0]) because gsl func wants non const double data[]
    gsl_fft_halfcomplex_inverse(yhc.data(), 1, yOutSize, wavetable, workspace);
    gsl_fft_halfcomplex_wavetable_free(wavetable);
    gsl_fft_real_workspace_free(workspace);

    // finally, setup the output workspace
    auto xOutSize = inWS->isHistogramData() ? yOutSize + 1 : yOutSize;
    outWS = WorkspaceFactory::Instance().create(inWS, 1, xOutSize, yOutSize);
    auto tAxis = std::make_unique<API::TextAxis>(1);
    tAxis->setLabel(0, "Real");
    outWS->replaceAxis(1, std::move(tAxis));

    df = 1.0 / (dx * static_cast<double>(yOutSize));

    auto &xData = outWS->mutableX(0);
    auto &yData = outWS->mutableY(0);

    std::generate(xData.begin(), xData.end(), HistogramData::LinearGenerator(0, df));
    std::move(yhc.begin(), yhc.end(), yData.begin());
    yData /= df;
  }

  setProperty("OutputWorkspace", outWS);
}

} // namespace Mantid::Algorithms
