// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {

// forward declarations
namespace API {
class Axis;
}

namespace Algorithms {
/**
This algorithm "transposes" the bins of the input workspace into a single
spectra. So, given
an input workspace of N1 Spectra with N2 Bins, the result is a workspace with N2
Spectra, and
N1 Bins. The output workspace is data points, not histograms.

@author Michael Whitty, STFC ISIS
@date 09/09/2010
 */
class MANTID_ALGORITHMS_DLL Transpose final : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "Transpose"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Transposes a workspace, so that an N1 x N2 workspace becomes N2 x "
           "N1.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"TransposeMD", "ConvertUnits", "ConvertSpectrumAxis", "ConvertAxesToRealSpace"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Axes"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Create the output workspace
  API::MatrixWorkspace_sptr createOutputWorkspace(const API::MatrixWorkspace_const_sptr &inputWorkspace);
  /// Return the vertical axis on the workspace, throwing if it is not valid
  API::Axis *getVerticalAxis(const API::MatrixWorkspace_const_sptr &workspace) const;
};

} // namespace Algorithms
} // namespace Mantid
