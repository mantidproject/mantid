#ifndef MANTID_ALGORITHMS_SPATIAL_GROUPING_H_
#define MANTID_ALGORITHMS_SPATIAL_GROUPING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
/// Forward Declarations
class IDetector;
class BoundingBox;
}

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
  /// (Empty) Constructor
  SpatialGrouping() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SpatialGrouping() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SpatialGrouping"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "This algorithm creates an XML grouping file, which can be used in "
           "GroupDetectors or ReadGroupsFromFile, which groups the detectors "
           "of an instrument based on the distance between the detectors. It "
           "does this by querying the getNeighbours method on the Detector "
           "object.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Transforms\\Grouping"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  /// expand our search out to the next neighbours along
  bool expandNet(std::map<specid_t, Mantid::Kernel::V3D> &nearest,
                 specid_t spec, const size_t &noNeighbours,
                 const Mantid::Geometry::BoundingBox &bbox);
  /// sort by distance
  void sortByDistance(std::map<specid_t, Mantid::Kernel::V3D> &nearest,
                      const size_t &noNeighbours);
  /// create expanded bounding box for our purposes
  void createBox(boost::shared_ptr<const Geometry::IDetector> det,
                 Geometry::BoundingBox &bndbox);
  /// grow dimensions of our bounding box to the factor
  void growBox(double &min, double &max, const double &factor);

  /// map of detectors in the instrument
  std::map<specid_t, boost::shared_ptr<const Geometry::IDetector>> m_detectors;
  /// flag which detectors are included in a group already
  std::set<specid_t> m_included;
  /// first and last values for each group
  std::vector<std::vector<int>> m_groups;
  /// number of pixels to search through for finding group
  double m_pix;

  /// Source workspace
  Mantid::API::MatrixWorkspace_const_sptr inputWorkspace;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SPATIAL_GROUPING_H_*/
