// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CalculateSensitivity_H_
#define MANTID_ALGORITHMS_CalculateSensitivity_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

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
class DLLExport CalculateSensitivity : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CalculateSensitivity"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the detector efficiency for a SANS instrument.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "SANS;CorrectionFunctions\\EfficiencyCorrections";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// Sum all detectors, excluding monitors and masked detectors
  std::tuple<double, double, int>
      sumUnmaskedAndDeadPixels(API::MatrixWorkspace_sptr);

  void averageAndNormalizePixels(API::MatrixWorkspace_sptr,
                                 std::tuple<double, double, int>);

  void applyBadPixelThreshold(API::MatrixWorkspace_sptr, double, double);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CalculateSensitivity_H_*/
