// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FFTDerivative.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/BoundedValidator.h"

#include <algorithm>
#include <functional>

using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FFTDerivative)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

void FFTDerivative::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "Input workspace for differentiation");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Workspace with result derivatives");
  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  declareProperty("Order", 1, mustBePositive, "The order of the derivative");
  // declareProperty("Transform",false,"Output the transform workspace");
}

void FFTDerivative::exec() { execComplexFFT(); }

void FFTDerivative::execComplexFFT() {
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outWS;

  size_t n = inWS->getNumberHistograms();
  API::Progress progress(this, 0.0, 1.0, n);

  size_t ny = inWS->y(0).size();
  size_t nx = inWS->x(0).size();

  // Workspace for holding a copy of a spectrum. Each spectrum is symmetrized to
  // minimize
  // possible edge effects.

  HistogramBuilder builder;
  builder.setX(nx + ny);
  builder.setY(ny + ny);
  builder.setDistribution(inWS->isDistribution());
  MatrixWorkspace_sptr copyWS =
      create<MatrixWorkspace>(*inWS, 1, builder.build());

  for (size_t spec = 0; spec < n; ++spec) {
    symmetriseSpectrum(inWS->histogram(spec), copyWS->mutableX(0),
                       copyWS->mutableY(0), nx, ny);

    // Transform symmetrized spectrum
    const bool isHisto = copyWS->isHistogramData();
    IAlgorithm_sptr fft = createChildAlgorithm("FFT");
    fft->setProperty("InputWorkspace", copyWS);
    fft->setProperty("Real", 0);
    fft->setProperty("Transform", "Forward");
    fft->execute();

    MatrixWorkspace_sptr transWS = fft->getProperty("OutputWorkspace");

    multiplyTransform(transWS->mutableX(3), transWS->mutableY(3),
                      transWS->mutableY(4));

    // Inverse transform
    fft = createChildAlgorithm("FFT");
    fft->setProperty("InputWorkspace", transWS);
    fft->setProperty("Real", 3);
    fft->setProperty("Imaginary", 4);
    fft->setProperty("Transform", "Backward");
    fft->execute();

    transWS = fft->getProperty("OutputWorkspace");

    // If the input was histogram data, convert the output to histogram data too
    if (isHisto) {
      IAlgorithm_sptr toHisto = createChildAlgorithm("ConvertToHistogram");
      toHisto->setProperty("InputWorkspace", transWS);
      toHisto->execute();
      transWS = toHisto->getProperty("OutputWorkspace");
    }

    if (!outWS) {
      outWS = create<MatrixWorkspace>(*inWS);
    }

    // Save the upper half of the inverse transform for output
    size_t m2 = transWS->y(0).size() / 2;
    double dx = copyWS->x(0)[m2];

    outWS->mutableX(spec).assign(transWS->x(0).cbegin() + m2,
                                 transWS->x(0).cend());
    outWS->mutableX(spec) += dx;
    outWS->mutableY(spec).assign(transWS->y(0).cbegin() + m2,
                                 transWS->y(0).cend());

    progress.report();
  }

  setProperty("OutputWorkspace", outWS);
}

void FFTDerivative::symmetriseSpectrum(const HistogramData::Histogram &in,
                                       HistogramData::HistogramX &symX,
                                       HistogramData::HistogramY &symY,
                                       const size_t nx, const size_t ny) {
  auto &inX = in.x();
  auto &inY = in.y();

  double xx = 2 * inX[0];

  symX[ny] = inX[0];
  symY[ny] = inY[0];

  for (size_t i = 1; i < ny; ++i) {
    size_t j1 = ny - i;
    size_t j2 = ny + i;
    symX[j1] = xx - inX[i];
    symX[j2] = inX[i];
    symY[j1] = symY[j2] = inY[i];
  }

  symX[0] = 2 * symX[1] - symX[2];
  symY[0] = inY.back();

  bool isHist = (nx != ny);

  if (isHist) {
    symX[symY.size()] = inX[ny];
  }
}

/** A Fourier transform of a derivative of order `n` has a factor of `i^n` where
 * `i` is the imaginary unit. This code multiplies the Fourier transform of the
 * input function `(re[j], im[j])` by `(2*pi*nu)^n` without using
 * `std::complex`.
 * @param nu :: complete real X of input histogram
 * @param &re :: complete real Y  of input histogram
 * @param &im :: complete imaginary Y of input histogram
 */
void FFTDerivative::multiplyTransform(HistogramX &nu, HistogramY &re,
                                      HistogramY &im) {
  int dn = getProperty("Order");
  bool swap_re_im = dn % 2 != 0;
  int sign_re = 1;
  int sign_im = -1;
  switch (dn % 4) {
  case 1:
    sign_re = 1;
    sign_im = -1;
    break;
  case 2:
    sign_re = -1;
    sign_im = -1;
    break;
  case 3:
    sign_re = -1;
    sign_im = 1;
    break;
  }
  // Multiply the transform by (2*pi*i*w)**dn
  for (size_t j = 0; j < re.size(); ++j) {
    double w = 2 * M_PI * nu[j];
    double ww = w;
    for (int k = dn; k > 1; --k) {
      ww *= w;
    }
    double a = sign_re * re[j] * ww;
    double b = sign_im * im[j] * ww;
    if (swap_re_im) {
      re[j] = b;
      im[j] = a;
    } else {
      re[j] = a;
      im[j] = b;
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
