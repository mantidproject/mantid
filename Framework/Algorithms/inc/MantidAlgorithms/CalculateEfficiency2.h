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
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace {
struct SummedResults {
  double sum{0.0};
  double error{0.0};
  size_t nPixels{0};
};
} // namespace

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

    <LI> MinSensitivity - Minimum efficiency for a pixel to be considered
 (default: no minimum)</LI>
    <LI> MaxSensitivity - Maximum efficiency for a pixel to be considered
 (default: no maximum)</LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_ALGORITHMS_DLL CalculateEfficiency2 : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CalculateEfficiency"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Calculates the detector efficiency for a SANS instrument."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (2); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "SANS;CorrectionFunctions\\EfficiencyCorrections"; }

private:
  // Overridden Algorithm methods
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  bool processGroups() override;

  API::MatrixWorkspace_sptr calculateEfficiency(API::MatrixWorkspace_sptr, double startProgress = 0.0,
                                                double stepProgress = 1.0);

  /// Sum all detectors, excluding monitors and masked detectors
  SummedResults sumUnmaskedAndDeadPixels(const API::MatrixWorkspace &workspace);

  void averageAndNormalizePixels(API::MatrixWorkspace &workspace, const SummedResults &results);

  API::MatrixWorkspace_sptr mergeGroup(API::WorkspaceGroup &);
  void validateGroupInput();

  /// Minimum efficiency. Pixels with lower efficiency will be masked
  double m_minThreshold{0.};
  /// Maximum efficiency. Pixels with higher efficiency will be masked
  double m_maxThreshold{2.};
};

} // namespace Algorithms
} // namespace Mantid
