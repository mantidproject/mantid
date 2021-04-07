// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/PointGroup.h"

namespace Mantid {
namespace Crystal {

/** Statistics of a PeaksWorkspace
 * @author Vickie Lynch, SNS
 * @date 2015-01-05
 */

class MANTID_CRYSTAL_DLL StatisticsOfPeaksWorkspace : public API::Algorithm {
public:
  StatisticsOfPeaksWorkspace();
  /// Algorithm's name for identification
  const std::string name() const override { return "StatisticsOfPeaksWorkspace"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Statistics of a PeaksWorkspace."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"ShowPeakHKLOffsets"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks;DataHandling\\Text"; }

private:
  /// Point Groups possible
  std::vector<Mantid::Geometry::PointGroup_sptr> m_pointGroups;
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Runs SortHKL on workspace
  void doSortHKL(const Mantid::API::Workspace_sptr &ws, const std::string &runName);

  DataObjects::PeaksWorkspace_sptr ws;
};

} // namespace Crystal
} // namespace Mantid
