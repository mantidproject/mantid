// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidIndexing/SpectrumNumber.h"

namespace Mantid {
namespace Algorithms {
/**
 Algorithm to focus powder diffraction data into a number of histograms
 according to a
 grouping scheme defined in a file.
 The structure of the grouping file is as follows:
 # Format: number  UDET offset  select  group
 0        611  0.0000000  1    0
 1        612  0.0000000  1    0
 2        601  0.0000000  0    0
 3        602  0.0000000  0    0
 4        621  0.0000000  1    0
 The first column is simply an index, the second is a UDET identifier for the
 detector,
 the third column corresponds to an offset in Deltad/d (not applied, usually
 applied using
 the ConvertUnits algorithm). The forth column is a flag to indicate whether
 the detector
 is selected. The fifth column indicates the group this detector belongs to
 (number >=1),
 zero is not considered as a group.

 Given an InputWorkspace and a Grouping filename, the algorithm follows:
 1) The calibration file is read and a map of corresponding udet-group is
 created.
 2) The algorithm determine the X boundaries for each group as the upper and
 lower limits
 of all contributing detectors to this group and determine a logarithmic step
 that will ensure
 preserving the number of bins in the initial workspace.
 3) All histograms are read and rebinned to the new grid for their group.
 4) A new workspace with N histograms is created.

 Since the new X boundaries depend on the group and not the entire workspace,
 this focusing algorithm does not create overestimated Xranges for multi-group
 intruments.

 Required Properties:
 <UL>
 <LI> InputWorkspace - The name of the 2D Workspace to take as input.
 It should be an histogram and the X-unit should be d-spacing. </LI>
 <LI> GroupingFileName - The path to a grouping file</LI>
 <LI> OutputWorkspace - The name of the 2D workspace in which to store the
 result </LI>
 </UL>

 @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
 @date 08/03/2009
 */
class MANTID_ALGORITHMS_DLL DiffractionFocussing2 final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "DiffractionFocussing"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Algorithm to focus powder diffraction data into a number of "
           "histograms according to a grouping scheme defined in a CalFile.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override {
    return {"AlignDetectors", "AlignAndFocusPowder", "LoadCalFile"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\Focussing"; }

private:
  // Overridden Algorithm methods
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  void cleanup();

  void getGroupingWorkspace();
  std::size_t setupGroupToWSIndices();

  // For events
  void execEvent();

  /// Loop over the workspace and determine the rebin parameters
  /// (Xmin,Xmax,step) for each group.
  /// The result is stored in group2params
  void determineRebinParameters(const std::vector<int> &udet2group);
  void determineRebinParametersFromParameters(const std::vector<int> &udet2group);
  int validateSpectrumInGroup(const std::vector<int> &udet2group, size_t wi);

  /// Shared pointer to the input workspace
  API::MatrixWorkspace_const_sptr m_matrixInputW;

  /// Grouping workspace with groups to build
  Mantid::DataObjects::GroupingWorkspace_sptr m_groupWS;

  // This map needs to be ordered to process the groups in order.
  /// typedef for the storage of each group's X vector
  using group2vectormap = std::map<int, std::shared_ptr<MantidVec>>;
  /// The list of group numbers
  std::vector<int> groupAtWorkspaceIndex;
  /// Map from the group number to the group's X vector
  std::map<int, HistogramData::BinEdges> group2xvector;
  std::map<int, double> group2xstep;
  /// Map from the group number to the group's summed weight vector
  group2vectormap group2wgtvector;
  /// The number of (used) groups
  int64_t nGroups = 0;
  /// Number of histograms
  int nHist = 0;
  /// Number of points in the 2D workspace
  int nPoints = 0;
  /// Mapping of group number to vector of inputworkspace indices.
  std::vector<std::vector<std::size_t>> m_wsIndices;
  /// List of valid group numbers
  std::vector<Indexing::SpectrumNumber> m_validGroups;
};

} // namespace Algorithms
} // namespace Mantid
