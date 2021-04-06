// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {

namespace HistogramData {
class Histogram;
class HistogramX;
class HistogramY;
} // namespace HistogramData

namespace Algorithms {

/** Calculates derivatives of the spectra in a MatrixWorkspace using a Fast
   Fourier Transform.

    @author Roman Tolchenov
    @date 21/02/2011
 */
class FFTDerivative : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "FFTDerivative"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculated derivatives of a spectra in the MatrixWorkspace using "
           "Fast Fourier Transform (FFT).";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractFFTSpectrum", "FFT", "MaxEnt", "RealFFT", "SassenaFFT", "FFTSmooth"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Arithmetic\\FFT"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  void execComplexFFT();
  void symmetriseSpectrum(const HistogramData::Histogram &in, HistogramData::HistogramX &symX,
                          HistogramData::HistogramY &symY, const size_t nx, const size_t ny);
  void multiplyTransform(HistogramData::HistogramX &nu, HistogramData::HistogramY &re, HistogramData::HistogramY &im);
};

} // namespace Algorithms
} // namespace Mantid
