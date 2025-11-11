// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
namespace FFTSmooth {
/** Data smoothing using the FFT algorithm and various filters.

    @author Roman Tolchenov
    @date 07/07/2009
 */
class MANTID_ALGORITHMS_DLL FFTSmooth2 : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FFTSmooth"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Performs smoothing of a spectrum using various filters."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"FFT", "WienerSmooth"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\FFT;Transforms\\Smoothing"; }

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
inline std::string const FILTER("Filter");
inline std::string const PARAMS("Params");
inline std::string const IGNORE_X_BINS("IgnoreXBins");
inline std::string const ALL_SPECTRA("AllSpectra");
} // namespace PropertyNames

} // namespace FFTSmooth
} // namespace Algorithms
} // namespace Mantid
