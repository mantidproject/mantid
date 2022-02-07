// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceNearestNeighbourInfo.h"
#include "MantidAlgorithms/DllConfig.h"
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
*/
class MANTID_ALGORITHMS_DLL SpatialGrouping : public API::Algorithm {
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
  const std::vector<std::string> seeAlso() const override { return {"GroupDetectors"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Grouping"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// expand our search out to the next neighbours along
  bool expandNet(std::map<specnum_t, Mantid::Kernel::V3D> &nearest, specnum_t spec, const size_t noNeighbours,
                 const Mantid::Geometry::BoundingBox &bbox);
  /// sort by distance
  void sortByDistance(std::map<specnum_t, Mantid::Kernel::V3D> &nearest, const size_t noNeighbours);
  /// create expanded bounding box for our purposes
  void createBox(const Geometry::IDetector &det, Geometry::BoundingBox &bndbox, double searchDist);
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
