// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/WorkspaceJoiners.h"

// Forward declarations
namespace Mantid {
namespace DataObjects {
class EventWorkspace;
}
} // namespace Mantid

namespace Mantid {
namespace Algorithms {
/** Joins two partial, non-overlapping workspaces into one. Used in the
   situation where you
    want to load a raw file in two halves, process the data and then join them
   back into
    a single dataset.
    The input workspaces must come from the same instrument, have common units
   and bins
    and no detectors that contribute to spectra should overlap.

    Required Properties:
    <UL>
    <LI> InputWorkspace1  - The name of the first input workspace. </LI>
    <LI> InputWorkspace2  - The name of the second input workspace. </LI>
    </UL>

    The output will be stored in the first named input workspace, the second
   will be deleted.

    @author Russell Taylor, Tessella
    @date 25/08/2008
*/
class MANTID_ALGORITHMS_DLL ConjoinWorkspaces : public WorkspaceJoiners {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ConjoinWorkspaces"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"ConjoinSpectra", "ConjoinXRuns", "MergeRuns"}; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  bool checkBinning(const API::MatrixWorkspace_const_sptr &ws1, const API::MatrixWorkspace_const_sptr &ws2) const;
  void checkForOverlap(const API::MatrixWorkspace &ws1, const API::MatrixWorkspace &ws2, bool checkSpectra) const;
  API::MatrixWorkspace_sptr conjoinEvents(const DataObjects::EventWorkspace &ws1,
                                          const DataObjects::EventWorkspace &ws2);
  API::MatrixWorkspace_sptr conjoinHistograms(const API::MatrixWorkspace &ws1, const API::MatrixWorkspace &ws2);
  void fixSpectrumNumbers(const API::MatrixWorkspace &ws1, const API::MatrixWorkspace &ws2,
                          API::MatrixWorkspace &output) override;
  bool processGroups() override;
  void setYUnitAndLabel(API::MatrixWorkspace &ws) const;

  /// True if spectra overlap
  bool m_overlapChecked = false;
};

} // namespace Algorithms
} // namespace Mantid
