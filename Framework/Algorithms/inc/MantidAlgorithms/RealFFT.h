// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_REALFFT_H_
#define MANTID_ALGORITHM_REALFFT_H_

#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAlgorithms/FFT.h"

namespace Mantid {
namespace Algorithms {
/** Performs a Fast Fourier Transform of real data

    @author Roman Tolchenov
    @date 01/10/2009
 */
class DLLExport RealFFT : public API::ParallelAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RealFFT"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs real Fast Fourier Transform";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractFFTSpectrum", "FFT",      "FFTDerivative", "MaxEnt",
            "SassenaFFT",         "FFTSmooth"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_REALFFT_H_*/
