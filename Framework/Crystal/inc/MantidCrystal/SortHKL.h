#ifndef MANTID_CRYSTAL_SORTHKL_H_
#define MANTID_CRYSTAL_SORTHKL_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Crystal/PointGroup.h"
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
  ~SortHKL();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "SortHKL"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Sorts a PeaksWorkspace by HKL. Averages intensities using point "
           "group.";
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
  void Outliers(std::vector<double> &data, std::vector<double> &err);

  /// Rounds a double using 0.5 as the cut off for rounding down
  double round(double d);
  /// Rounds the V3D to integer values
  Kernel::V3D round(Kernel::V3D hkl);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SORTHKL_H_ */
