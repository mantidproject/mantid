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

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if
 */
void RealFFT::exec() {
  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  std::string transform = getProperty("Transform");
  bool IgnoreXBins = getProperty("IgnoreXBins");

  int spec = (transform == "Forward") ? getProperty("WorkspaceIndex") : 0;

  const auto &X = inWS->x(spec);
  auto ySize = static_cast<int>(inWS->blocksize());

  if (spec >= ySize)
    throw std::invalid_argument("Property WorkspaceIndex is out of range");

  // Check that the x values are evenly spaced
  double dx = (X.back() - X.front()) / static_cast<double>(X.size() - 1);
  if (!IgnoreXBins) {
    for (size_t i = 0; i < X.size() - 2; i++)
      // note this cannot be replaced with Kernel::withinRelativeDifference,
      // or fails to detect some errors
      if (std::abs(dx - X[i + 1] + X[i]) / dx > 1e-7)
        throw std::invalid_argument("X axis must be linear (all bins have same "
                                    "width). This can be ignored if "
                                    "IgnoreXBins is set to true.");
  }

  API::MatrixWorkspace_sptr outWS;

  double df = 1.0 / (dx * ySize);

  if (transform == "Forward") {
    int yOutSize = ySize / 2 + 1;
    int xOutSize = inWS->isHistogramData() ? yOutSize + 1 : yOutSize;
    bool odd = ySize % 2 != 0;

    outWS = WorkspaceFactory::Instance().create(inWS, 3, xOutSize, yOutSize);
    auto tAxis = std::make_unique<API::TextAxis>(3);
    tAxis->setLabel(0, "Real");
    tAxis->setLabel(1, "Imag");
    tAxis->setLabel(2, "Modulus");
    outWS->replaceAxis(1, std::move(tAxis));

    gsl_fft_real_workspace *workspace = gsl_fft_real_workspace_alloc(ySize);
    std::vector<double> data(2 * ySize);

    auto &yData = inWS->mutableY(spec);
    for (int i = 0; i < ySize; i++) {
      data[i] = yData[i];
    }

    gsl_fft_real_wavetable *wavetable = gsl_fft_real_wavetable_alloc(ySize);
    gsl_fft_real_transform(data.data(), 1, ySize, wavetable, workspace);
    gsl_fft_real_wavetable_free(wavetable);
    gsl_fft_real_workspace_free(workspace);

    auto &x = outWS->mutableX(0);
    auto &y1 = outWS->mutableY(0);
    auto &y2 = outWS->mutableY(1);
    auto &y3 = outWS->mutableY(2);
    for (int i = 0; i < yOutSize; i++) {
      int j = i * 2;
      x[i] = df * i;
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

    int yOutSize = (ySize - 1) * 2;
    if (inWS->y(1).back() != 0.0)
      yOutSize++;
    int xOutSize = inWS->isHistogramData() ? yOutSize + 1 : yOutSize;
    bool odd = yOutSize % 2 != 0;

    df = 1.0 / (dx * (yOutSize));

    outWS = WorkspaceFactory::Instance().create(inWS, 1, xOutSize, yOutSize);
    auto tAxis = std::make_unique<API::TextAxis>(1);
    tAxis->setLabel(0, "Real");
    outWS->replaceAxis(1, std::move(tAxis));

    gsl_fft_real_workspace *workspace = gsl_fft_real_workspace_alloc(yOutSize);

    auto &xData = outWS->mutableX(0);
    auto &yData = outWS->mutableY(0);
    auto &y0 = inWS->mutableY(0);
    auto &y1 = inWS->mutableY(1);
    for (int i = 0; i < ySize; i++) {
      int j = i * 2;
      xData[i] = df * i;
      if (i != 0) {
        yData[j - 1] = y0[i];
        if (odd || i != ySize - 1) {
          yData[j] = y1[i];
        }
      } else {
        yData[0] = y0[0];
      }
    }

    gsl_fft_halfcomplex_wavetable *wavetable = gsl_fft_halfcomplex_wavetable_alloc(yOutSize);

    // &(yData[0]) because gsl func wants non const double data[]
    gsl_fft_halfcomplex_inverse(&(yData[0]), 1, yOutSize, wavetable, workspace);
    gsl_fft_halfcomplex_wavetable_free(wavetable);
    gsl_fft_real_workspace_free(workspace);

    std::generate(xData.begin(), xData.end(), HistogramData::LinearGenerator(0, df));
    yData /= df;

    if (outWS->isHistogramData())
      outWS->mutableX(0)[yOutSize] = outWS->mutableX(0)[yOutSize - 1] + df;
  }

  setProperty("OutputWorkspace", outWS);
}

} // namespace Mantid::Algorithms
