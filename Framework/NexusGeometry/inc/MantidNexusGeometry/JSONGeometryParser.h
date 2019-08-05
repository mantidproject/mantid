// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_JSONGEOMETRYPARSER_H_
#define MANTID_NEXUSGEOMETRY_JSONGEOMETRYPARSER_H_

#include "MantidNexusGeometry/DllConfig.h"
#include <Eigen/Geometry>
#include <json/json.h>
#include <memory>
#include <vector>

namespace Mantid {

namespace NexusGeometry {
struct Chopper {
  std::string componentName;
  std::string name;
  std::vector<double> slitEdges;
  double radius;
  double slitHeight;
  int64_t slits;
  std::string tdcTopic;
  std::string tdcSource;
  std::string tdcWriterModule;
};

struct Monitor {
  std::string componentName;
  std::string name;
  int64_t detectorID;
  // monitor stream
  std::string eventStreamTopic;
  std::string eventStreamSource;
  std::string eventStreamWriterModule;
  std::string waveformTopic;
  std::string waveformSource;
  std::string waveformWriterModule;

  // monitor transformation
  Eigen::Vector3d translation;
  Eigen::Quaterniond orientation;
  // monitor shape
  std::vector<Eigen::Vector3d> vertices;
  std::vector<int32_t> cylinders;
  std::vector<int32_t> faces;
  std::vector<int32_t> windingOrder;
  bool isOffGeometry;
};

struct Detector {
  std::string name;
  std::string componentName;
  std::string detectorID;
  Eigen::Vector3d position;
  Eigen::Vector3d translation;
  Eigen::Quaterniond orientation;
  // detector shape
  std::vector<Eigen::Vector3d> vertices;
  std::vector<int32_t> cylinders;
  std::vector<int32_t> faces;
  std::vector<int32_t> windingOrder;
  bool isOffGeometry;
};

/** JSONGeometryParser : Parses a JSON string which has a parallel structure to
   nexus geometry in nexus files and extracts all information about the
   instrument. https://www.nexusformat.org/

   @see NexusGeometryParser
 */
class MANTID_NEXUSGEOMETRY_DLL JSONGeometryParser {
public:
  JSONGeometryParser() = delete;
  JSONGeometryParser(const std::string &json);
  ~JSONGeometryParser() = default;
  size_t size() noexcept { return m_jsonDetectorBanks.size(); }
  const std::vector<int64_t> &detectorIDs(const size_t index) const noexcept {
    return m_detIDs[index];
  }

  const std::string &detectorName(const size_t index) const noexcept {
    return m_detectorBankNames[index];
  }

  const std::vector<Monitor> &monitors() const noexcept { return m_monitors; }

  const std::vector<Chopper> &choppers() const noexcept { return m_choppers; }

  const std::vector<double> &xPixelOffsets(const size_t index) const noexcept {
    return m_x[index];
  }

  const std::vector<double> &yPixelOffsets(const size_t index) const noexcept {
    return m_y[index];
  }

  const std::vector<double> &zPixelOffsets(const size_t index) const noexcept {
    return m_z[index];
  }
  const Eigen::Vector3d &translation(const size_t index) const noexcept {
    return m_translations[index];
  }

  const Eigen::Quaterniond &orientation(const size_t index) const noexcept {
    return m_orientations[index];
  }

  bool isOffGeometry(const size_t index) const noexcept {
    return m_isOffGeometry[index];
  }

  const std::vector<Eigen::Vector3d> &vertices(size_t index) const noexcept {
    return m_pixelShapeVertices[index];
  }

  const std::vector<int32_t> &faces(size_t index) const noexcept {
    return m_pixelShapeFaces[index];
  }

  const std::vector<int32_t> &windingOrder(size_t index) const noexcept {
    return m_pixelShapeWindingOrder[index];
  }

  const std::vector<int32_t> &cylinders(size_t index) const noexcept {
    return m_pixelShapeCylinders[index];
  }

  constexpr double degreesToRadians(const double degrees) noexcept;

private:
  /// Parse geometry provided with a string representing geometry.
  void parse(const std::string &jsonGeometry);
  /// clear all geometry information
  void reset() noexcept;
  void validateAndRetrieveGeometry(const std::string &jsonGeometry);
  void extractDetectorContent();
  void extractMonitorContent();
  void extractChopperContent();
  void extractTransformationDataset(const Json::Value &transformation,
                                    double &value, Eigen::Vector3d &axis);
  void extractTransformations(const Json::Value &transformations,
                              Eigen::Vector3d &translation,
                              Eigen::Quaterniond &orientation);

private:
  Json::Value m_root;
  Json::Value m_instrument;
  Json::Value m_sample;
  // monitor information
  std::vector<Json::Value> m_jsonMonitors;
  std::vector<Monitor> m_monitors;
  // chopper information
  std::vector<Json::Value> m_jsonChoppers;
  std::vector<Chopper> m_choppers;
  // detector information
  std::vector<Json::Value> m_jsonDetectorBanks;
  std::vector<Detector> m_detectors;
  std::vector<std::string> m_detectorBankNames;
  std::vector<std::vector<int64_t>> m_detIDs;
  std::vector<std::vector<double>> m_x;
  std::vector<std::vector<double>> m_y;
  std::vector<std::vector<double>> m_z;
  // pixel shapes
  std::vector<std::vector<int32_t>> m_pixelShapeFaces;
  std::vector<std::vector<int32_t>> m_pixelShapeCylinders;
  std::vector<std::vector<Eigen::Vector3d>> m_pixelShapeVertices;
  std::vector<std::vector<int32_t>> m_pixelShapeWindingOrder;
  std::vector<bool> m_isOffGeometry;

  std::vector<Eigen::Vector3d> m_translations;
  std::vector<Eigen::Quaterniond> m_orientations;
};

} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_JSONGEOMETRYPARSER_H_ */
