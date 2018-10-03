#ifndef MANTID_CRYSTAL_StatisticsOfPeaksWorkspace_H_
#define MANTID_CRYSTAL_StatisticsOfPeaksWorkspace_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** Statistics of a PeaksWorkspace
 * @author Vickie Lynch, SNS
 * @date 2015-01-05
 */

class DLLExport StatisticsOfPeaksWorkspace : public API::Algorithm {
public:
  StatisticsOfPeaksWorkspace();
  /// Algorithm's name for identification
  const std::string name() const override {
    return "StatisticsOfPeaksWorkspace";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Statistics of a PeaksWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"ShowPeakHKLOffsets"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\Peaks;DataHandling\\Text";
  }

private:
  /// Point Groups possible
  std::vector<Mantid::Geometry::PointGroup_sptr> m_pointGroups;
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Runs SortHKL on workspace
  void doSortHKL(Mantid::API::Workspace_sptr ws, std::string runName);

  DataObjects::PeaksWorkspace_sptr ws;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_StatisticsOfPeaksWorkspace_H_ */
