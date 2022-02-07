// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
/** Compares two workspaces for equality.
 *
 * This algorithm is deprecated. Please use CompareWorkspaces instead.
 *
 * The data values (X,Y and error) are always checked. The algorithm can also
 * optionally check
 * the axes (this includes the units), the spectra-detector map, the instrument
 * (the name and
 * parameter map) and any bin masking.
 *
 * Required Properties:
 * <UL>
 * <LI> Workspace1 - The name of the first input workspace </LI>
 * <LI> Workspace2 - The name of the second input workspace </LI>
 * </UL>
 *
 * Optional Properties:
 * <UL>
 * <LI> Tolerance       - The maximum amount by which values may differ between
 * the workspaces (default: 0) </LI>
 * <LI> CheckAxes       - Whether to check that the axes match (default: true)
 * </LI>
 * <LI> CheckSpectraMap - Whether to check that the spectra-detector maps match
 * (default: true) </LI>
 * <LI> CheckInstrument - Whether to check that the instruments match (default:
 * true) </LI>
 * <LI> CheckMasking    - Whether to check that the bin masking matches
 * (default: true) </LI>
 * <LI> CheckSample     - Whether to check that the sample object mathces
 * (default: false) </LI>
 * </UL>
 *
 * Output Properties:
 * <UL>
 * <LI> Result - Contains 'success' if the workspaces match, the reason for the
 * failure otherwise </LI>
 * </UL>
 *
 *
 */
class MANTID_ALGORITHMS_DLL CheckWorkspacesMatch : public API::Algorithm, public API::DeprecatedAlgorithm {
public:
  CheckWorkspacesMatch();

  /// Algorithm's name
  const std::string name() const override { return "CheckWorkspacesMatch"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Compares two workspaces for equality. This algorithm is mainly "
           "intended for use by the Mantid development team as part of the "
           "testing process.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"CompareWorkspaces"}; }

  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }

  /// The string that is returned when comparison is successful.
  static std::string successString() { return "Success!"; }

private:
  /// Initialisation code
  void init() override;

  /// Called when comparing two individual workspaces
  void exec() override;

  /// Called when comparing workspace groups
  bool processGroups() override;

  /// This algorithm is now deprecated and calls CompareWorkspaces instead
  std::string runCompareWorkspaces(bool group_compare = false);
};

} // namespace Algorithms
} // namespace Mantid
