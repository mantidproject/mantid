// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexusGeometry/JSONGeometryParser.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace {
const std::string NEXUS_STRUCTURE = "nexus_structure";
const std::string NXCLASS = "NX_class";
const std::string NXDETECTOR = "NXdetector";
const std::string NXMONITOR = "NXmonitor";
const std::string NXDISK_CHOPPER = "NXdisk_chopper";
const std::string NXINSTRUMENT = "NXinstrument";
const std::string NXSAMPLE = "NXsample";
const std::string NXENTRY = "NXentry";
const std::string NXTRANSFORMATIONS = "NXtransformations";
const std::string NXOFF_GEOMETRY = "NXoff_geometry";
const std::string NXCYLINDRICAL_GEOMETRY = "NXcylindrical_geometry";
const std::string DETECTOR_NUMBER = "detector_number";
const std::string PIXEL_SHAPE = "pixel_shape";
const std::string TRANSFORMATIONS = "transformations";
const std::string X_PIXEL_OFFSET = "x_pixel_offset";
const std::string Y_PIXEL_OFFSET = "y_pixel_offset";
const std::string DEPENDENCY = "depends_on";
const std::string CHILDREN = "children";
const std::string ATTRIBUTES = "attributes";
const std::string NAME = "name";
const std::string VALUES = "values";
constexpr double PI = 3.1415926535;

bool validateNXAttribute(const Json::Value &attributes,
                         const std::string &NXAttribute) {
  auto NXAttr = attributes[0];
  return (NXAttr[NAME] == NXCLASS && NXAttr[VALUES] == NXAttribute);
}

Json::Value get(const Json::Value &entry, const std::string &NXAttribute) {
  Json::Value item;
  for (const auto &child : entry) {
    auto attributes = child[ATTRIBUTES];
    if (!attributes.isNull() && validateNXAttribute(attributes, NXAttribute)) {
      item = child;
      break;
    }
  }
  return item;
}

std::vector<Json::Value> getAllNXComponents(const Json::Value &instrument,
                                            const std::string &NXClass) {
  std::vector<Json::Value> nxComponents;

  const auto &components = instrument[CHILDREN];
  for (const auto &component : components) {
    auto attributes = component[ATTRIBUTES];
    if (validateNXAttribute(attributes, NXClass))
      nxComponents.push_back(component);
  }

  return nxComponents;
}

std::vector<Json::Value> getAllDetectors(const Json::Value &instrument) {
  return getAllNXComponents(instrument, NXDETECTOR);
}

std::vector<Json::Value> getAllMonitors(const Json::Value &instrument) {
  return getAllNXComponents(instrument, NXMONITOR);
}

std::vector<Json::Value> getAllChoppers(const Json::Value &instrument) {
  return getAllNXComponents(instrument, NXDISK_CHOPPER);
}

template <class T>
void addSingleValue(const Json::Value &val, std::vector<T> &fillArray) {
  if (std::is_same<T, std::int64_t>::value)
    fillArray.push_back(static_cast<T>(val.asInt64()));
  else if (std::is_same<T, std::uint64_t>::value)
    fillArray.push_back(static_cast<T>(val.asUInt64()));
  else if (std::is_same<T, std::int32_t>::value)
    fillArray.push_back(static_cast<T>(val.asInt()));
  else if (std::is_same<T, std::uint32_t>::value)
    fillArray.push_back(static_cast<T>(val.asUInt()));
  else if (std::is_same<T, float>::value)
    fillArray.push_back(static_cast<T>(val.asFloat()));
  else if (std::is_same<T, double>::value)
    fillArray.push_back(static_cast<T>(val.asDouble()));
}

/// Recursively fills JSON array data trees which are usually arranges as arrays
/// of arrays.
template <class T>
void recursiveFill(const Json::Value &jsonArray, std::vector<T> &fillArray) {
  if (!jsonArray.isArray()) {
    addSingleValue(jsonArray, fillArray);
  }

  for (const auto &val : jsonArray) {
    if (val.isArray())
      recursiveFill<T>(val, fillArray);
    else {
      addSingleValue(val, fillArray);
    }
  }
}

/// Recursively search through the "child" tree structure to find dependencies.
/// When a match is made at a particular level, the list which represents the
/// path is truncated. If the values are completely emptied, then the path has
/// been found.
void recursiveDependencySearch(const Json::Value &parent,
                               std::vector<std::string> &values) {
  auto name = parent[NAME].asString();
  const auto &children = parent[CHILDREN];

  if (values.size() > 0 && parent[NAME] == values.back())
    values.pop_back();

  if (children.isNull())
    return;

  for (const auto &child : children)
    recursiveDependencySearch(child, values);
}

template <class T>
void extractDatasetValues(const Json::Value &datasetParent,
                          std::vector<T> &data) {
  auto datadesc = datasetParent["dataset"];
  auto shape = datadesc["size"];
  auto values = datasetParent[VALUES];

  auto insertIndex = data.size();
  std::vector<uint32_t> dims(shape.size());

  size_t nValues = 1;
  for (Json::ArrayIndex i = 0; i < shape.size(); ++i) {
    dims[i] = shape[i].asUInt();
    nValues *= dims[i];
  }

  data.reserve(data.size() + nValues);

  auto outer = dims[0];
  for (Json::ArrayIndex i = 0; i < outer; ++i) {
    auto val = values[i];
    recursiveFill<T>(val, data);
  }
}

void getPixelShapeInformation(const Json::Value &pixShape,
                              std::vector<int32_t> &cylinders,
                              std::vector<int32_t> &faces,
                              std::vector<Eigen::Vector3d> &vertices,
                              std::vector<int32_t> &windingOrder,
                              bool &isOffGeometry) {
  auto attributes = pixShape[ATTRIBUTES][0];
  std::vector<float> verts;

  isOffGeometry = false;
  if (attributes[VALUES] == NXOFF_GEOMETRY) {
    for (const auto &child : pixShape[CHILDREN]) {
      if (child[NAME] == "faces")
        extractDatasetValues<int32_t>(child, faces);
      else if (child[NAME] == "vertices")
        extractDatasetValues<float>(child, verts);
      else if (child[NAME] == "winding_order")
        extractDatasetValues<int32_t>(child, windingOrder);
    }

    if (windingOrder.size() != verts.size() / 3)
      throw std::invalid_argument(
          "Invalid off geometry provided in json pixel_shape.");

    isOffGeometry = true;
  }

  if (attributes[VALUES] == NXCYLINDRICAL_GEOMETRY) {
    for (const auto &child : pixShape[CHILDREN]) {
      if (child[NAME] == "cylinders")
        extractDatasetValues<int32_t>(child, cylinders);
      else if (child[NAME] == "vertices")
        extractDatasetValues<float>(child, verts);
    }

    if (cylinders.size() != verts.size() / 3)
      throw std::invalid_argument(
          "Invalid cylindrical geometry provided in json pixel_shape.");
  }

  auto insertIndex = vertices.size();
  vertices.reserve(vertices.size() + (verts.size() / 3));

  for (size_t i = 0; i < verts.size(); i += 3)
    vertices.push_back(Eigen::Vector3d(static_cast<double>(verts[i]),
                                       static_cast<double>(verts[i + 1]),
                                       static_cast<double>(verts[i + 2])));
}

void verifyDependency(const Json::Value &root, const Json::Value &dependency) {
  auto path = dependency[VALUES].asString();

  if (path == "." || path.empty())
    return;

  std::vector<std::string> values;
  boost::split(values, path, boost::is_any_of("/"), boost::token_compress_on);
  std::reverse(values.begin(), values.end());
  values.pop_back(); // remove empty first value
  // Search dependency tree and verify all items present.
  // removes value from vector once found.
  recursiveDependencySearch(root[NEXUS_STRUCTURE], values);

  // Left over values suggests the dependency could not be found
  if (values.size() > 0)
    throw std::invalid_argument("Could not find dependency in json provided.");
}

void getTransformationAttributeVector(const Json::Value &root,
                                      const Json::Value &attributes,
                                      std::vector<double> &vec) {
  auto size = attributes.size();
  for (const auto &attribute : attributes) {
    if (attribute[NAME] == DEPENDENCY)
      verifyDependency(root, attribute);
    else if (attribute[NAME] == "vector") {
      const auto &values = attribute[VALUES];
      vec.resize(values.size());
      std::transform(values.begin(), values.end(), vec.begin(),
                     [](const Json::Value &val) { return val.asDouble(); });
    }
  }
}

void extractStream(const Json::Value &group, std::string &topic,
                   std::string &source, std::string &writerModule) {
  const auto &children = group["children"];

  for (const auto &child : children) {
    if (child["type"] == "stream") {
      const auto &stream = child["stream"];
      topic = stream["topic"].asString();
      source = stream["source"].asString();
      writerModule = stream["writer_module"].asString();
    }
  }
}

void extractChopperTDC(
    const Json::Value &tdc,
    Mantid::NexusGeometry::JSONGeometryParser::ChopperInfo &info) {
  extractStream(tdc, info.tdcTopic, info.tdcSource, info.tdcWriterModule);
}

void extractMonitorEventStream(
    const Json::Value &events,
    Mantid::NexusGeometry::JSONGeometryParser::MonitorInfo &info) {
  extractStream(events, info.eventStreamTopic, info.eventStreamSource,
                info.eventStreamWriterModule);
}
void extractMonitorWaveformStream(
    const Json::Value &waveform,
    Mantid::NexusGeometry::JSONGeometryParser::MonitorInfo &info) {
  extractStream(waveform, info.waveformTopic, info.waveformSource,
                info.waveformWriterModule);
}

Json::Value getRoot(const std::string &jsonGeometry) {
  if (jsonGeometry.empty())
    throw std::invalid_argument("Empty geometry json string provided.");

  Json::CharReaderBuilder rbuilder;
  auto reader = rbuilder.newCharReader();
  Json::Value root;
  std::string errors;

  reader->parse(&jsonGeometry.front(), &jsonGeometry.back(), &root, &errors);

  return root;
}

} // namespace

namespace Mantid {
namespace NexusGeometry {

void JSONGeometryParser::reset() noexcept {
  m_detectors.clear();
  m_detectorNames.clear();
  m_detIDs.clear();
  m_pixelShapeCylinders.clear();
  m_pixelShapeFaces.clear();
  m_pixelShapeVertices.clear();
  m_pixelShapeWindingOrder.clear();
  m_translations.clear();
  m_orientations.clear();
  m_x.clear();
  m_y.clear();
  m_isOffGeometry.clear();
}

void JSONGeometryParser::validateAndRetrieveGeometry(
    const std::string &jsonGeometry) {
  auto root = getRoot(jsonGeometry);
  auto nexusStructure = root[NEXUS_STRUCTURE];

  if (nexusStructure.isNull())
    throw std::invalid_argument(
        "Json geometry does not contain nexus_structure.");

  auto nexusChildren = nexusStructure[CHILDREN];

  auto entry = nexusChildren[0]; // expect children to be array type
  if (entry.isNull() || !validateNXAttribute(entry[ATTRIBUTES], NXENTRY))
    throw std::invalid_argument(
        "No nexus \"entry\" child found in nexus_structure json.");

  auto entryChildren = entry[CHILDREN];

  auto sample = get(entryChildren, NXSAMPLE);
  if (sample.isNull())
    throw std::invalid_argument("No sample found in json.");

  auto instrument = get(entryChildren, NXINSTRUMENT);
  if (instrument.isNull())
    throw std::invalid_argument("No instrument found in json.");

  m_detectors = getAllDetectors(instrument);
  if (m_detectors.size() == 0)
    throw std::invalid_argument("No detectors found in json.");

  m_monitors = getAllMonitors(instrument);

  m_choppers = getAllChoppers(instrument);

  for (const auto &detector : m_detectors)
    m_detectorNames.push_back(detector[NAME].asString());

  m_root = root;
  m_sample = sample;
  m_instrument = instrument;
}

/** Extract detailed information
 */
void JSONGeometryParser::extractTransformationDataset(
    const Json::Value &transformation, double &value, Eigen::Vector3d &axis) {
  std::vector<double> values;
  extractDatasetValues(transformation, values);
  std::vector<double> vec;
  getTransformationAttributeVector(m_root, transformation[ATTRIBUTES], vec);
  axis = Eigen::Vector3d(vec[0], vec[1], vec[2]);
  value = values[0];
}

/** Extract all detector transformations.
 */
void JSONGeometryParser::extractTransformations(
    const Json::Value &transformations, Eigen::Vector3d &translation,
    Eigen::Quaterniond &orientation) {
  Eigen::Vector3d location(0, 0, 0);
  Eigen::Vector3d beamDirectionOffset(0, 0, 0);
  Eigen::Vector3d orientationVector(0, 0, 0);
  double angle = 0.0;

  auto &children = transformations[CHILDREN];

  for (const auto &transformation : children) {
    double value;
    if (transformation[NAME] == "location") {
      extractTransformationDataset(transformation, value, location);
      location *= value;
    }
    else if (transformation[NAME] == "beam_direction_offset") {
      extractTransformationDataset(transformation, value, beamDirectionOffset);
      beamDirectionOffset *= value;
    }
    else if (transformation[NAME] == "orientation")
      extractTransformationDataset(transformation, angle, orientationVector);
  }
  translation = location + beamDirectionOffset;
  orientation = Eigen::AngleAxisd(degreesToRadians(angle),
                                  Eigen::Vector3d(orientationVector));
}

/** Extract contents of all detectors found in the instrument and stores
 * information like positions, detector shapes etc.
 */
void JSONGeometryParser::extractDetectorContent() {
  for (const auto &detector : m_detectors) {
    std::vector<int64_t> detIDs;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<int> cylinders;
    std::vector<int> faces;
    std::vector<Eigen::Vector3d> vertices;
    std::vector<int> windingOrder;
    bool isOffGeometry = false;

    auto children = detector[CHILDREN];

    for (const auto &child : children) {
      if (child[NAME] == DETECTOR_NUMBER)
        extractDatasetValues<int64_t>(child, detIDs);
      else if (child[NAME] == X_PIXEL_OFFSET)
        extractDatasetValues<double>(child, x);
      else if (child[NAME] == Y_PIXEL_OFFSET)
        extractDatasetValues<double>(child, y);
      else if (child[NAME] == PIXEL_SHAPE)
        getPixelShapeInformation(child, cylinders, faces, vertices,
                                 windingOrder, isOffGeometry);
      else if (child[NAME] == DEPENDENCY)
        verifyDependency(m_root, child);
      else if (validateNXAttribute(child[ATTRIBUTES], NXTRANSFORMATIONS)) {
        m_translations.push_back(Eigen::Vector3d());
        m_orientations.push_back(Eigen::Quaterniond());
        extractTransformations(child, m_translations.back(),
                               m_orientations.back());
      }
    }

    if (detIDs.size() == 0)
      throw std::invalid_argument("No detector ids found in json.");
    if (x.size() == 0)
      throw std::invalid_argument("No x_pixel_offsets found in json.");
    if (y.size() == 0)
      throw std::invalid_argument("No y_pixel_offsets found in json.");
    if ((isOffGeometry && (vertices.size() == 0 || faces.size() == 0 ||
                           windingOrder.size() == 0)) ||
        (!isOffGeometry && (vertices.size() == 0 || cylinders.size() == 0)))
      throw std::invalid_argument(
          "Insufficient pixel shape information found in json.");

    m_detIDs.emplace_back(std::move(detIDs));
    m_x.emplace_back(std::move(x));
    m_y.emplace_back(std::move(y));
    m_isOffGeometry.push_back(isOffGeometry);
    m_pixelShapeCylinders.emplace_back(std::move(cylinders));
    m_pixelShapeVertices.emplace_back(std::move(vertices));
    m_pixelShapeFaces.emplace_back(std::move(faces));
    m_pixelShapeWindingOrder.emplace_back(std::move(windingOrder));
  }
}

void JSONGeometryParser::extractMonitorContent() {
  if (m_monitors.size() == 0)
    return;

  for (const auto &monitor : m_monitors) {
    const auto &children = monitor[CHILDREN];

    if (children.size() == 0)
      throw std::invalid_argument(
          "Full monitor definition missing in json provided.");

    MonitorInfo info;
    info.componentName = monitor[NAME].asString();
    for (const auto &child : children) {
      const auto &val = child[VALUES];
      if (child[NAME] == "name")
        info.name = val.asString();
      else if (child[NAME] == "detector_id")
        info.detectorID = val.asInt64();
      else if (child[NAME] == "events")
        extractMonitorEventStream(child, info);
      else if (child[NAME] == "waveforms")
        extractMonitorWaveformStream(child, info);
      else if (child[NAME] == "transformations")
        extractTransformations(child, info.translation, info.orientation);
      else if (child[NAME] == "depends_on")
        verifyDependency(m_root, child);
    }

    m_monitorInfos.emplace_back(std::move(info));
  }
}

/** Extract contents of all choppers found in the instrument and stores
 * information like pslit_edges, top_dead_center stream etc.
 */
void JSONGeometryParser::extractChopperContent() {
  if (m_choppers.size() == 0)
    return;

  for (const auto &chopper : m_choppers) {
    const auto &children = chopper[CHILDREN];

    if (children.size() == 0)
      throw std::invalid_argument(
          "Full chopper definition missing in json provided.");
    ChopperInfo info;
    info.componentName = chopper[NAME].asString();
    for (const auto &child : children) {
      const auto &val = child[VALUES];
      if (child[NAME] == "name")
        info.name = val.asString();
      else if (child[NAME] == "slit_edges")
        extractDatasetValues(child, info.slitEdges);
      else if (child[NAME] == "slit_height")
        info.slitHeight = val.asDouble();
      else if (child[NAME] == "radius")
        info.radius = val.asDouble();
      else if (child[NAME] == "slits")
        info.slits = val.asInt64();
      else if (child[NAME] == "top_dead_center")
        extractChopperTDC(child, info);
    }

    m_chopperInfos.emplace_back(std::move(info));
  }
}

/** Parses instrument geometry which is formated in json corresponding to the
hdf5 nexus structure.
@param jsonGeometry - JSON string representing the nexus style instrument
@throws std::invalid_argument if the geometry string is invalid.
*/
void JSONGeometryParser::parse(const std::string &jsonGeometry) {
  // Throws when there are issues with the geometry. Performs shallow test for
  // NXEntry, NXSample, NXInstrument and all NXDetector instances.
  validateAndRetrieveGeometry(jsonGeometry);
  extractMonitorContent();
  extractChopperContent();
  extractDetectorContent();
}

constexpr double
JSONGeometryParser::degreesToRadians(const double degrees) noexcept {
  return degrees * PI / 180.0;
}
} // namespace NexusGeometry
} // namespace Mantid
