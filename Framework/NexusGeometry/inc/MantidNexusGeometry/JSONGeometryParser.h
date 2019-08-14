// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_JSONGEOMETRYPARSER_H_
#define MANTID_NEXUSGEOMETRY_JSONGEOMETRYPARSER_H_

#include "MantidGeometry/IDTypes.h"
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
  uint64_t slits;
  std::string tdcTopic;
  std::string tdcSource;
  std::string tdcWriterModule;
};

struct Monitor {
  std::string componentName;
  std::string name;
  detid_t detectorID;
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
  std::vector<uint32_t> cylinders;
  std::vector<uint32_t> faces;
  std::vector<uint32_t> windingOrder;
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
  const std::string &name() const noexcept { return m_name; }
  const std::string &sampleName() const noexcept { return m_sampleName; }
  const std::string &sourceName() const noexcept { return m_sourceName; }
  const Eigen::Vector3d &samplePosition() const noexcept {
    return m_samplePosition;
  }
  const Eigen::Quaterniond &sampleOrientation() const noexcept {
    return m_sampleOrientation;
  }
  const Eigen::Vector3d &sourcePosition() const noexcept {
    return m_sourcePosition;
  }
  const Eigen::Quaterniond &sourceOrientation() const noexcept {
    return m_sourceOrientation;
  }
  size_t numberOfBanks() const noexcept { return m_jsonDetectorBanks.size(); }
  const std::vector<detid_t> &detectorIDs(const size_t index) const noexcept {
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

  const std::vector<uint32_t> &faces(size_t index) const noexcept {
    return m_pixelShapeFaces[index];
  }

  const std::vector<uint32_t> &windingOrder(size_t index) const noexcept {
    return m_pixelShapeWindingOrder[index];
  }

  const std::vector<uint32_t> &cylinders(size_t index) const noexcept {
    return m_pixelShapeCylinders[index];
  }

  double degreesToRadians(const double degrees) noexcept;

private:
  /// Parse geometry provided with a string representing geometry.
  void parse(const std::string &jsonGeometry);
  void validateAndRetrieveGeometry(const std::string &jsonGeometry);
  void extractSampleContent();
  void extractSourceContent();
  void extractDetectorContent();
  void extractMonitorContent();
  void extractChopperContent();
  void extractTransformationDataset(const Json::Value &transformation,
                                    double &value, Eigen::Vector3d &axis);
  void extractTransformations(const Json::Value &transformations,
                              Eigen::Vector3d &translation,
                              Eigen::Quaterniond &orientation);

private:
  std::string m_name;
  Json::Value m_root;
  Json::Value m_instrument;
  Json::Value m_sample;
  Json::Value m_source;
  std::string m_sampleName;
  std::string m_sourceName;
  Eigen::Vector3d m_samplePosition;
  Eigen::Quaterniond m_sampleOrientation;
  Eigen::Vector3d m_sourcePosition;
  Eigen::Quaterniond m_sourceOrientation;
  // monitor information
  std::vector<Json::Value> m_jsonMonitors;
  std::vector<Monitor> m_monitors;
  // chopper information
  std::vector<Json::Value> m_jsonChoppers;
  std::vector<Chopper> m_choppers;
  // detector information
  std::vector<Json::Value> m_jsonDetectorBanks;
  std::vector<std::string> m_detectorBankNames;
  std::vector<std::vector<detid_t>> m_detIDs;
  std::vector<std::vector<double>> m_x;
  std::vector<std::vector<double>> m_y;
  std::vector<std::vector<double>> m_z;
  // pixel shapes
  std::vector<std::vector<uint32_t>> m_pixelShapeFaces;
  std::vector<std::vector<uint32_t>> m_pixelShapeCylinders;
  std::vector<std::vector<Eigen::Vector3d>> m_pixelShapeVertices;
  std::vector<std::vector<uint32_t>> m_pixelShapeWindingOrder;
  std::vector<bool> m_isOffGeometry;

  std::vector<Eigen::Vector3d> m_translations;
  std::vector<Eigen::Quaterniond> m_orientations;
};

} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_JSONGEOMETRYPARSER_H_ */
