//
// Created by michael on 23/08/17.
//

#ifndef MANTID_NEXUS_GEOMETRY_PARSER_H_
#define MANTID_NEXUS_GEOMETRY_PARSER_H_

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <H5Cpp.h>
#include "MantidNexusGeometry/DllConfig.h"
#include <memory>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {
class IObject;
class Instrument;
}
namespace NexusGeometry {
class InstrumentBuilder;

// TODO change class to namespace
class DLLExport NexusGeometryParser {
public:
  std::unique_ptr<const Mantid::Geometry::Instrument>
  createInstrument(const std::string &fileName) const;

private:
  std::unique_ptr<const Mantid::Geometry::Instrument>
  extractInstrument(const H5::H5File &file, const H5::Group &root) const;
  /// Opens sub groups of current group
  std::vector<H5::Group> openSubGroups(const H5::Group &parentGroup,
                                       const H5std_string &CLASS_TYPE) const;
  /// Opens all detector groups in a file
  std::vector<H5::Group> openDetectorGroups(const H5::Group &root) const;
  /// Stores detectorGroup detIds in vector of ints
  std::vector<int> getDetectorIds(const H5::Group &detectorGroup) const;
  /// Stores detectorGroup pixel offsets as Eigen 3xN matrix
  Eigen::Matrix<double, 3, Eigen::Dynamic>
  getPixelOffsets(const H5::Group &detectorGroup) const;
  /// Gets the transformations applied to the detector's pixelOffsets
  Eigen::Transform<double, 3, Eigen::Affine>
  getTransformations(const H5::H5File &file,
                     const H5::Group &detectorGroup) const;
  /// Gets the data from a string dataset
  H5std_string get1DStringDataset(const H5std_string &dataset,
                                  const H5::Group &group) const;
  /// Read dataset into vector
  template <typename valueType>
  std::vector<valueType> get1DDataset(const H5::H5File &file,
                                      const H5std_string &dataset) const;
  template <typename valueType>
  std::vector<valueType> get1DDataset(const H5std_string &dataset,
                                      const H5::Group &group) const;
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>
  parsePixelShape(const H5::Group &detectorGroup) const;
  /// Parse shape - choose what type shape
  boost::shared_ptr<const Geometry::IObject>
  parseNexusShape(const H5::Group &detectorGroup) const;
  /// Parse cylinder nexus geometry
  boost::shared_ptr<const Geometry::IObject>
  parseNexusCylinder(const H5::Group &shapeGroup) const;
  /// Parse OFF (mesh) nexus geometry
  boost::shared_ptr<const Geometry::IObject>
  parseNexusMesh(const H5::Group &shapeGroup) const;
  /// Parse source
  void parseAndAddSource(const H5::H5File &file, const H5::Group &root,
                         InstrumentBuilder &builder) const;
  /// Parse sample
  void parseAndAddSample(const H5::H5File &file, const H5::Group &root,
                         InstrumentBuilder &builder) const;
  /// Parse monitors
  void parseMonitors(const H5::Group &root, InstrumentBuilder &builder) const;

  /// From the NeXus-OFF polygonal mesh, create a triangular mesh
  std::vector<uint16_t>
  createTriangularFaces(const std::vector<uint16_t> &faceIndices,
                        const std::vector<uint16_t> &windingOrder) const;

  void createTrianglesFromPolygon(const std::vector<uint16_t> &windingOrder,
                                  std::vector<uint16_t> &triangularFaces,
                                  int &startOfFace, int &endOfFace) const;
  std::string instrumentName(const H5::Group &root) const;
};
} // namespace NexusGeometry
} // namespace Mantid

#endif // MANTID_NEXUS_GEOMETRY_PARSER_H_
