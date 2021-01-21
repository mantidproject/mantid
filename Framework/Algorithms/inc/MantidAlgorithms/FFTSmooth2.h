// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Data smoothing using the FFT algorithm and various filters.

    @author Roman Tolchenov
    @date 07/07/2009
 */
class MANTID_ALGORITHMS_DLL FFTSmooth2 : public API::ParallelAlgorithm {
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
  void exec() override;

  // Smoothing by zeroing.
  void zero(int n, API::MatrixWorkspace_sptr &unfilteredWS, API::MatrixWorkspace_sptr &filteredWS);
  // Smoothing using Butterworth filter of any positive order.
  void Butterworth(int n, int order, API::MatrixWorkspace_sptr &unfilteredWS, API::MatrixWorkspace_sptr &filteredWS);
};

} // namespace Algorithms
} // namespace Mantid
