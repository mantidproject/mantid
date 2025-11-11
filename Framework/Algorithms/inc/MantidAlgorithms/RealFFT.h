// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAlgorithms/FFT.h"

namespace Mantid {
namespace Algorithms {
namespace RealFFT {
/** Performs a Fast Fourier Transform of real data

    @author Roman Tolchenov
    @date 01/10/2009
 */
class MANTID_ALGORITHMS_DLL RealFFT : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RealFFT"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Performs real Fast Fourier Transform"; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractFFTSpectrum", "FFT", "FFTDerivative", "MaxEnt", "SassenaFFT", "FFTSmooth"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT"; }

private:
  // Overridden Algorithm methods
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  std::map<std::string, std::string> actuallyValidateInputs(API::Workspace_sptr const &);
  void exec() override;
};

namespace PropertyNames {
inline std::string const INPUT_WKSP("InputWorkspace");
inline std::string const OUTPUT_WKSP("OutputWorkspace");
inline std::string const WKSP_INDEX("WorkspaceIndex");
inline std::string const TRANSFORM("Transform");
inline std::string const IGNORE_X_BINS("IgnoreXBins");
} // namespace PropertyNames
} // namespace RealFFT
} // namespace Algorithms
} // namespace Mantid
