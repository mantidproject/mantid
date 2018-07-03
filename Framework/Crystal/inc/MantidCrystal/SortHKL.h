#ifndef MANTID_CRYSTAL_SORTHKL_H_
#define MANTID_CRYSTAL_SORTHKL_H_

#include "MantidKernel/System.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include "MantidCrystal/PeakStatisticsTools.h"

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/UnitCell.h"

#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Crystal {

/** Save a PeaksWorkspace to a Gsas-style ASCII .hkl file.
 *
 * @author Vickie Lynch, SNS
 * @date 2012-01-20
 */
class DLLExport SortHKL : public API::Algorithm {
public:
  SortHKL();
  ~SortHKL() override;

  /// Algorithm's name for identification
  const std::string name() const override { return "SortHKL"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Sorts a PeaksWorkspace by HKL. Averages intensities using point "
           "group.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"TransformHKL"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return R"(Crystal\Peaks;DataHandling\Text;Utility\Sorting)";
  }

private:
  void init() override;
  void exec() override;

  std::vector<DataObjects::Peak>
  getNonZeroPeaks(const std::vector<DataObjects::Peak> &inputPeaks) const;

  PeakStatisticsTools::UniqueReflectionCollection
  getUniqueReflections(const std::vector<DataObjects::Peak> &peaks,
                       const Geometry::UnitCell &cell) const;

  Geometry::ReflectionCondition_sptr getCentering() const;
  Geometry::PointGroup_sptr getPointgroup() const;

  std::pair<double, double>
  getDLimits(const std::vector<DataObjects::Peak> &peaks,
             const Geometry::UnitCell &cell) const;

  API::ITableWorkspace_sptr getStatisticsTable(const std::string &name) const;
  void insertStatisticsIntoTable(
      const API::ITableWorkspace_sptr &table,
      const PeakStatisticsTools::PeaksStatistics &statistics) const;

  DataObjects::PeaksWorkspace_sptr getOutputPeaksWorkspace(
      const DataObjects::PeaksWorkspace_sptr &inputPeaksWorkspace) const;

  void sortOutputPeaksByHKL(API::IPeaksWorkspace_sptr outputPeaksWorkspace);

  /// Point Groups possible
  std::vector<Mantid::Geometry::PointGroup_sptr> m_pointGroups;

  /// Reflection conditions
  std::vector<Mantid::Geometry::ReflectionCondition_sptr> m_refConds;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SORTHKL_H_ */
