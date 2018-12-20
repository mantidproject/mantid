#ifndef MANTID_ALGORITHMS_SPATIAL_GROUPING_H_
#define MANTID_ALGORITHMS_SPATIAL_GROUPING_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceNearestNeighbourInfo.h"
#include "MantidGeometry/IDTypes.h"

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class IDetector;
class BoundingBox;
} // namespace Geometry

namespace Algorithms {
/**
    This algorithm creates an XML Grouping File for use in the GroupDetectors
   (v2) or
    ReadGroupsFromFile algorithms. It does this by querying the
   NearestNeighbours in
    Geometry through the detector's parameter map.

    @author Michael Whitty, STFC ISIS
    @date 13/12/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SpatialGrouping : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SpatialGrouping"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm creates an XML grouping file, which can be used in "
           "GroupDetectors or ReadGroupsFromFile, which groups the detectors "
           "of an instrument based on the distance between the detectors. It "
           "does this by querying the getNeighbours method on the Detector "
           "object.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"GroupDetectors"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Grouping"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// expand our search out to the next neighbours along
  bool expandNet(std::map<specnum_t, Mantid::Kernel::V3D> &nearest,
                 specnum_t spec, const size_t noNeighbours,
                 const Mantid::Geometry::BoundingBox &bbox);
  /// sort by distance
  void sortByDistance(std::map<specnum_t, Mantid::Kernel::V3D> &nearest,
                      const size_t noNeighbours);
  /// create expanded bounding box for our purposes
  void createBox(const Geometry::IDetector &det, Geometry::BoundingBox &bndbox,
                 double searchDist);
  /// grow dimensions of our bounding box to the factor
  void growBox(double &min, double &max, const double factor);

  /// map of detectors in the instrument
  std::map<specnum_t, Kernel::V3D> m_positions;
  /// flag which detectors are included in a group already
  std::set<specnum_t> m_included;
  /// first and last values for each group
  std::vector<std::vector<int>> m_groups;

  /// NearestNeighbourInfo used by expandNet()
  std::unique_ptr<API::WorkspaceNearestNeighbourInfo> m_neighbourInfo;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SPATIAL_GROUPING_H_*/
