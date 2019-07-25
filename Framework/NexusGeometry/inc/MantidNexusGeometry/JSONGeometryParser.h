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

/** JSONGeometryParser : Parses a JSON string which has a parallel structure to
   nexus files and extracts all information about the instrument.
 */
class MANTID_NEXUSGEOMETRY_DLL JSONGeometryParser {
public:
  struct ChopperInfo {
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

  struct MonitorInfo {
    std::string componentName;
    std::string name;
    int64_t detectorID;
    std::string eventStreamTopic;
    std::string eventStreamSource;
    std::string eventStreamWriterModule;
    Eigen::Vector3d translation;
    Eigen::Quaterniond orientation;
    std::string waveformTopic;
    std::string waveformSource;
    std::string waveformWriterModule;
  };

  JSONGeometryParser() noexcept {};
  ~JSONGeometryParser() = default;
  /// Parse geometry provided with a string representing geometry.
  void parse(const std::string &jsonGeometry);
  size_t size() noexcept { return m_detectors.size(); }
  const std::vector<int64_t> &detectorIDs(const size_t index) const noexcept {
    return m_detIDs[index];
  }

  const std::string &detectorName(const size_t index) const noexcept {
    return m_detectorNames[index];
  }

  const std::vector<MonitorInfo> &monitors() const noexcept {
    return m_monitorInfos;
  }

  const std::vector<ChopperInfo> &choppers() const noexcept {
    return m_chopperInfos;
  }

  const std::vector<double> &xPixelOffsets(const size_t index) const noexcept {
    return m_x[index];
  }
  const std::vector<double> &yPixelOffsets(const size_t index) const noexcept {
    return m_y[index];
  }

  const Eigen::Vector3d &translation(const size_t index) const noexcept {
    return m_translations[index];
  }

  const Eigen::Quaterniond &orientation(const size_t index) const noexcept {
    return m_orientations[index];
  }

  const bool isOffGeometry(const size_t index) const noexcept {
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
  /// clear all geometry information
  void reset() noexcept;

private:
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
  std::vector<Json::Value> m_monitors;
  std::vector<MonitorInfo> m_monitorInfos;
  // chopper information
  std::vector<Json::Value> m_choppers;
  std::vector<ChopperInfo> m_chopperInfos;
  // detector information
  std::vector<Json::Value> m_detectors;
  std::vector<std::string> m_detectorNames;
  std::vector<std::vector<int64_t>> m_detIDs;
  std::vector<std::vector<double>> m_x;
  std::vector<std::vector<double>> m_y;
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