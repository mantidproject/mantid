// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
 *
    Compute relative detector pixel efficiency from flood data as part of SANS
 reduction.
    Normalizes pixel counts to the sums up all unmasked pixel counts. If a
 minimum and/or maximum
    maximum efficiency is provided, the  pixels falling outside the limits will
 be taken out
    of the normalization and masked.

    For workspaces with more than one TOF bins, the bins are summed up before
 the calculation
    and the resulting efficiency has a single TOF bin.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
 result </LI>
    </UL>

    Optional Properties:
    <UL>

    <LI> MinEfficiency - Minimum efficiency for a pixel to be considered
 (default: no minimum)</LI>
    <LI> MaxEfficiency - Maximum efficiency for a pixel to be considered
 (default: no maximum)</LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_ALGORITHMS_DLL CalculateEfficiency : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CalculateEfficiency"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Calculates the detector efficiency for a SANS instrument."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "SANS;CorrectionFunctions\\EfficiencyCorrections"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Sum all detectors, excluding monitors and masked detectors
  void sumUnmaskedDetectors(const API::MatrixWorkspace_sptr &rebinnedWS, double &sum, double &error, int &nPixels);

  /// Normalize all detectors to get the relative efficiency
  void normalizeDetectors(const API::MatrixWorkspace_sptr &rebinnedWS, const API::MatrixWorkspace_sptr &outputWS,
                          double sum, double error, int nPixels, double min_eff, double max_eff);

  void maskComponent(API::MatrixWorkspace &ws, const std::string &componentName);
  void maskEdges(const API::MatrixWorkspace_sptr &ws, int high, int low, int left, int right,
                 const std::string &componentName);
};

} // namespace Algorithms
} // namespace Mantid
