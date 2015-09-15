#ifndef MANTID_CRYSTAL_StatisticsOfPeaksWorkspace_H_
#define MANTID_CRYSTAL_StatisticsOfPeaksWorkspace_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/PointGroup.h"

namespace Mantid {
namespace Crystal {

/** Statistics of a PeaksWorkspace
 * @author Vickie Lynch, SNS
 * @date 2015-01-05
 */

class DLLExport StatisticsOfPeaksWorkspace : public API::Algorithm {
public:
  StatisticsOfPeaksWorkspace();
  ~StatisticsOfPeaksWorkspace();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "StatisticsOfPeaksWorkspace"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Statistics of a PeaksWorkspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Crystal;DataHandling\\Text";
  }

private:
  /// Point Groups possible
  std::vector<Mantid::Geometry::PointGroup_sptr> m_pointGroups;
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  /// Runs SortHKL on workspace
  void doSortHKL(Mantid::API::Workspace_sptr ws, std::string runName);

  DataObjects::PeaksWorkspace_sptr ws;
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_StatisticsOfPeaksWorkspace_H_ */
