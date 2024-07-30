// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/JSONGeometryParser.h"
#include "MantidKernel/Logger.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"

#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <json/json.h>

using Json::Value;

namespace {
using namespace Mantid::NexusGeometry;
Mantid::Kernel::Logger g_log("JSONGeometryParser");

// Json-specific Constants
const std::string CHILDREN = "children";
const std::string ATTRIBUTES = "attributes";
const std::string NAME = "name";
const std::string VALUES = "values";

bool validateNXAttribute(const Json::Value &attributes, const std::string &NXAttribute) {
  auto NXAttr = attributes[0];
  return (NXAttr[NAME] == NX_CLASS && NXAttr[VALUES] == NXAttribute);
}

Json::Value get(const Json::Value &entry, const std::string &NXAttribute) {
  Json::Value item;
  for (const auto &child : entry) {
    const auto &attributes = child[ATTRIBUTES];
    if (!attributes.isNull() && validateNXAttribute(attributes, NXAttribute)) {
      item = child;
      break;
    }
  }
  return item;
}

std::vector<Json::Value> getAllNXComponents(const Json::Value &instrument, const std::string &NXClass) {
  std::vector<Json::Value> nxComponents;

  const auto &components = instrument[CHILDREN];
  for (const auto &component : components) {
    auto attributes = component[ATTRIBUTES];
    if (validateNXAttribute(attributes, NXClass))
      nxComponents.emplace_back(component);
  }

  return nxComponents;
}

std::vector<Json::Value> getAllDetectors(const Json::Value &instrument) {
  return getAllNXComponents(instrument, NX_DETECTOR);
}

std::vector<Json::Value> getAllMonitors(const Json::Value &instrument) {
  return getAllNXComponents(instrument, NX_MONITOR);
}

std::vector<Json::Value> getAllChoppers(const Json::Value &instrument) {
  return getAllNXComponents(instrument, NX_DISK_CHOPPER);
}

void addSingleValue(const Json::Value &val, std::vector<double> &fillArray) { fillArray.emplace_back(val.asDouble()); }

void addSingleValue(const Json::Value &val, std::vector<float> &fillArray) { fillArray.emplace_back(val.asFloat()); }

void addSingleValue(const Json::Value &val, std::vector<uint32_t> &fillArray) { fillArray.emplace_back(val.asUInt()); }

void addSingleValue(const Json::Value &val, std::vector<int32_t> &fillArray) { fillArray.emplace_back(val.asInt()); }

/// Recursively fills JSON array data trees which are usually arranges as arrays
/// of arrays.
template <class T> void recursiveFill(const Json::Value &jsonArray, std::vector<T> &fillArray) {
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
void recursiveDependencySearch(const Json::Value &parent, std::vector<std::string> &values) {
  if (!values.empty() && parent[NAME] == values.back())
    values.pop_back();

  if (!parent.isMember(CHILDREN))
    return;

  const auto &children = parent[CHILDREN];
  for (const auto &child : children)
    recursiveDependencySearch(child, values);
}

template <class T> void extractDatasetValues(const Json::Value &datasetParent, std::vector<T> &data) {
  auto datadesc = datasetParent["dataset"];
  auto shape = datadesc["size"];
  auto values = datasetParent[VALUES];

  if (shape.isNull())
    return addSingleValue(values, data);

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

void extractShapeInformation(const Json::Value &shape, std::vector<uint32_t> &cylinders, std::vector<uint32_t> &faces,
                             std::vector<Eigen::Vector3d> &vertices, std::vector<uint32_t> &windingOrder,
                             bool &isOffGeometry) {
  auto attributes = shape[ATTRIBUTES][0];
  auto name = shape["name"].asString();
  std::vector<float> verts;

  const auto &children = shape[CHILDREN];
  isOffGeometry = false;
  if (attributes[VALUES] == NX_OFF) {
    for (const auto &child : children) {
      if (child[NAME] == "faces")
        extractDatasetValues<uint32_t>(child, faces);
      else if (child[NAME] == "vertices")
        extractDatasetValues<float>(child, verts);
      else if (child[NAME] == "winding_order")
        extractDatasetValues<uint32_t>(child, windingOrder);
    }

    if (windingOrder.size() != verts.size() / 3)
      throw std::invalid_argument("Invalid off geometry provided in JSON " + name + ".");

    isOffGeometry = true;
  }

  if (attributes[VALUES] == NX_CYLINDER) {
    for (const auto &child : children) {
      if (child[NAME] == "cylinders")
        extractDatasetValues<uint32_t>(child, cylinders);
      else if (child[NAME] == "vertices")
        extractDatasetValues<float>(child, verts);
    }

    if (cylinders.size() != verts.size() / 3)
      throw std::invalid_argument("Invalid cylindrical geometry provided in JSON " + name + ".");
  }

  vertices.reserve(vertices.size() + (verts.size() / 3));

  for (size_t i = 0; i < verts.size(); i += 3)
    vertices.emplace_back(Eigen::Vector3d(static_cast<double>(verts[i]), static_cast<double>(verts[i + 1]),
                                          static_cast<double>(verts[i + 2])));
}

bool validateShapeInformation(const bool &isOffGeometry, const std::vector<Eigen::Vector3d> &vertices,
                              const std::vector<uint32_t> &cylinders, const std::vector<uint32_t> &faces,
                              const std::vector<uint32_t> &windingOrder) {
  if ((isOffGeometry && (vertices.empty() || faces.empty() || windingOrder.empty())) ||
      (!isOffGeometry && (vertices.empty() || cylinders.empty())))
    return false;

  return true;
}

void verifyDependency(const Json::Value &root, const Json::Value &dependency) {
  auto path = dependency[VALUES].asString();

  if (path == NO_DEPENDENCY || path.empty())
    return;

  std::vector<std::string> values;
  boost::split(values, path, boost::is_any_of("/"), boost::token_compress_on);
  std::reverse(values.begin(), values.end());
  values.pop_back(); // remove empty first value
  // Search dependency tree and verify all items present.
  // removes value from vector once found.
  recursiveDependencySearch(root[NEXUS_STRUCTURE], values);

  // Left over values suggests the dependency could not be found
  if (!values.empty())
    throw std::invalid_argument("Could not find dependency " + path + " in JSON provided.");
}

Eigen::Vector3d getTransformationAxis(const Json::Value &root, const Json::Value &attributes) {
  std::vector<double> axis;
  for (const auto &attribute : attributes) {
    if (attribute[NAME] == DEPENDS_ON)
      verifyDependency(root, attribute);
    else if (attribute[NAME] == "vector") {
      const auto &values = attribute[VALUES];
      axis.resize(values.size());
      std::transform(values.begin(), values.end(), axis.begin(), [](const Json::Value &val) { return val.asDouble(); });
    }
  }

  return Eigen::Vector3d(axis[0], axis[1], axis[2]);
}

void extractStream(const Json::Value &group, std::string &topic, std::string &source, std::string &writerModule) {
  const auto &children = group[CHILDREN];

  for (const auto &child : children) {
    if (child["type"] == "stream") {
      const auto &stream = child["stream"];
      topic = stream["topic"].asString();
      source = stream["source"].asString();
      writerModule = stream["writer_module"].asString();
    }
  }
}

void extractChopperTDC(const Json::Value &tdc, Mantid::NexusGeometry::Chopper &info) {
  extractStream(tdc, info.tdcTopic, info.tdcSource, info.tdcWriterModule);
}

void extractMonitorEventStream(const Json::Value &events, Mantid::NexusGeometry::Monitor &info) {
  extractStream(events, info.eventStreamTopic, info.eventStreamSource, info.eventStreamWriterModule);
}
void extractMonitorWaveformStream(const Json::Value &waveform, Mantid::NexusGeometry::Monitor &info) {
  extractStream(waveform, info.waveformTopic, info.waveformSource, info.waveformWriterModule);
}

Json::Value getRoot(const std::string &jsonGeometry) {
  if (jsonGeometry.empty())
    throw std::invalid_argument("Empty geometry JSON string provided.");

  Json::CharReaderBuilder rbuilder;
  auto reader = std::unique_ptr<Json::CharReader>(rbuilder.newCharReader());
  Json::Value root;
  std::string errors;

  reader->parse(&jsonGeometry.front(), &jsonGeometry.back(), &root, &errors);

  return root;
}

std::string extractInstrumentName(const Json::Value &instrument) {
  std::string name;
  const auto &children = instrument[CHILDREN];
  const auto it =
      std::find_if(std::cbegin(children), std::cend(children), [](const auto child) { return child[NAME] == NAME; });
  if (it != std::cend(children))
    name = (*it)["values"].asString();

  return name;
}

std::vector<std::unique_ptr<Json::Value>> moveToUniquePtrVec(std::vector<Json::Value> &jsonVector) {
  std::vector<std::unique_ptr<Json::Value>> ret;
  std::transform(jsonVector.cbegin(), jsonVector.cend(), std::back_inserter(ret),
                 [](const auto &val) { return std::move(std::make_unique<Json::Value>(std::move(val))); });
  return ret;
}

} // namespace

namespace Mantid::NexusGeometry {

JSONGeometryParser::JSONGeometryParser(const std::string &json) { parse(json); }

void JSONGeometryParser::validateAndRetrieveGeometry(const std::string &jsonGeometry) {
  auto root = getRoot(jsonGeometry);
  auto nexusStructure = root[NEXUS_STRUCTURE];

  if (nexusStructure.isNull())
    throw std::invalid_argument("JSON geometry does not contain nexus_structure.");

  auto nexusChildren = nexusStructure[CHILDREN];

  auto entry = get(nexusChildren, NX_ENTRY); // expect children to be array type
  if (entry.isNull())
    throw std::invalid_argument("No nexus \"entry\" child found in nexus_structure JSON.");

  auto entryChildren = entry[CHILDREN];

  auto sample = get(entryChildren, NX_SAMPLE);
  if (sample.isNull())
    throw std::invalid_argument("No sample found in JSON.");

  auto instrument = get(entryChildren, NX_INSTRUMENT);
  if (instrument.isNull())
    throw std::invalid_argument("No instrument found in JSON.");

  m_name = extractInstrumentName(instrument);

  auto instrumentChildren = instrument[CHILDREN];

  auto source = get(instrumentChildren, NX_SOURCE);

  if (source.isNull())
    g_log.notice() << "No source information found in JSON instrument." << std::endl;

  auto jsonDetectorBanks = getAllDetectors(instrument);
  if (jsonDetectorBanks.empty())
    throw std::invalid_argument("No detectors found in JSON.");
  ;
  m_jsonDetectorBanks = moveToUniquePtrVec(jsonDetectorBanks);

  auto instrMonitors = getAllMonitors(instrument);
  auto entryMonitors = getAllMonitors(entry);
  instrMonitors.insert(instrMonitors.end(), std::make_move_iterator(entryMonitors.begin()),
                       std::make_move_iterator(entryMonitors.end()));
  m_jsonMonitors = moveToUniquePtrVec(instrMonitors);
  auto jsonChoppers = getAllChoppers(instrument);
  m_jsonChoppers = moveToUniquePtrVec(jsonChoppers);

  m_root = std::make_unique<Json::Value>(std::move(root));
  m_source = std::make_unique<Json::Value>(std::move(source));
  m_sample = std::make_unique<Json::Value>(std::move(sample));
  m_instrument = std::make_unique<Json::Value>(std::move(instrument));
}

void JSONGeometryParser::extractSampleContent() {
  const auto &children = (*m_sample)[CHILDREN];
  m_samplePosition = Eigen::Vector3d(0, 0, 0);
  m_sampleOrientation = Eigen::Quaterniond(Eigen::AngleAxisd(0, Eigen::Vector3d(1, 0, 0)));
  m_sampleName = (*m_sample)[NAME].asString();
  for (const auto &child : children) {
    if (validateNXAttribute(child[ATTRIBUTES], NX_TRANSFORMATIONS))
      extractTransformations(child, m_samplePosition, m_sampleOrientation);
  }
}

void JSONGeometryParser::extractSourceContent() {
  m_sourceName = "Unspecified";
  m_sourcePosition = Eigen::Vector3d(0, 0, 0);
  m_sourceOrientation = Eigen::Quaterniond(Eigen::AngleAxisd(0, Eigen::Vector3d(1, 0, 0)));
  if (!m_source->isNull()) {
    m_sourceName = (*m_source)[NAME].asCString();
    const auto &children = (*m_source)[CHILDREN];
    for (const auto &child : children) {
      if (validateNXAttribute(child[ATTRIBUTES], NX_TRANSFORMATIONS))
        extractTransformations(child, m_sourcePosition, m_sourceOrientation);
    }
  }
}

/** Extract detailed transformation information
 */
void JSONGeometryParser::extractTransformationDataset(const Json::Value &transformation, double &value,
                                                      Eigen::Vector3d &axis) {
  std::vector<double> values;
  extractDatasetValues(transformation, values);
  axis = getTransformationAxis(*m_root, transformation[ATTRIBUTES]);
  value = values[0];
}

/** Extract all detector transformations.
 */
void JSONGeometryParser::extractTransformations(const Json::Value &transformations, Eigen::Vector3d &translation,
                                                Eigen::Quaterniond &orientation) {
  Eigen::Vector3d location(0, 0, 0);
  Eigen::Vector3d beamDirectionOffset(0, 0, 0);
  Eigen::Vector3d orientationVector(0, 0, 1);
  double angle = 0.0;

  const auto &children = transformations[CHILDREN];

  for (const auto &transformation : children) {
    double value;
    if (transformation[NAME] == "location") {
      extractTransformationDataset(transformation, value, location);
      location *= value;
    } else if (transformation[NAME] == "beam_direction_offset") {
      extractTransformationDataset(transformation, value, beamDirectionOffset);
      beamDirectionOffset *= value;
    } else if (transformation[NAME] == "orientation") {
      extractTransformationDataset(transformation, angle, orientationVector);
    }
  }
  translation = location + beamDirectionOffset;
  orientation = Eigen::AngleAxisd(degreesToRadians(angle), Eigen::Vector3d(orientationVector));
}

/** Extract contents of all detectors found in the instrument and stores
 * information like positions, detector shapes etc.
 */
void JSONGeometryParser::extractDetectorContent() {
  for (const auto &detector : m_jsonDetectorBanks) {
    std::vector<detid_t> detIDs;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> z;
    std::vector<uint32_t> cylindersVec;
    std::vector<uint32_t> facesVec;
    std::vector<Eigen::Vector3d> verticesVec;
    std::vector<uint32_t> windingOrderVec;
    bool isOffGeometryShape = false;

    auto children = (*detector)[CHILDREN];

    for (const auto &child : children) {
      if (child[NAME] == DETECTOR_IDS)
        extractDatasetValues<detid_t>(child, detIDs);
      else if (child[NAME] == X_PIXEL_OFFSET)
        extractDatasetValues<double>(child, x);
      else if (child[NAME] == Y_PIXEL_OFFSET)
        extractDatasetValues<double>(child, y);
      else if (child[NAME] == Z_PIXEL_OFFSET)
        extractDatasetValues<double>(child, z);
      else if (child[NAME] == PIXEL_SHAPE)
        extractShapeInformation(child, cylindersVec, facesVec, verticesVec, windingOrderVec, isOffGeometryShape);
      else if (child[NAME] == DEPENDS_ON)
        verifyDependency(*m_root, child);
      else if (validateNXAttribute(child[ATTRIBUTES], NX_TRANSFORMATIONS)) {
        m_translations.emplace_back(Eigen::Vector3d::Zero());
        m_orientations.emplace_back(Eigen::Quaterniond::Identity());
        extractTransformations(child, m_translations.back(), m_orientations.back());
      }
    }
    auto nameDetector = (*detector)[NAME].asString();
    if (detIDs.empty())
      throw std::invalid_argument("No detector ids found in " + nameDetector + ".");
    if (x.empty())
      throw std::invalid_argument("No x_pixel_offsets found in " + nameDetector + ".");
    if (y.empty())
      throw std::invalid_argument("No y_pixel_offsets found in " + nameDetector + ".");
    if (!validateShapeInformation(isOffGeometryShape, verticesVec, cylindersVec, facesVec, windingOrderVec))
      throw std::invalid_argument("Insufficient pixel shape information found in " + nameDetector + ".");

    m_detectorBankNames.emplace_back(nameDetector);
    m_detIDs.emplace_back(std::move(detIDs));
    m_x.emplace_back(std::move(x));
    m_y.emplace_back(std::move(y));
    m_z.emplace_back(std::move(z));
    m_isOffGeometry.emplace_back(isOffGeometryShape);
    m_pixelShapeCylinders.emplace_back(std::move(cylindersVec));
    m_pixelShapeVertices.emplace_back(std::move(verticesVec));
    m_pixelShapeFaces.emplace_back(std::move(facesVec));
    m_pixelShapeWindingOrder.emplace_back(std::move(windingOrderVec));
  }
}

void JSONGeometryParser::extractMonitorContent() {
  if (m_jsonMonitors.empty())
    return;

  int monitorID = -1;
  for (const auto &monitor : m_jsonMonitors) {
    const auto &children = (*monitor)[CHILDREN];
    auto nameMonitor = (*monitor)[NAME].asString();
    if (children.empty())
      throw std::invalid_argument("Full monitor definition for " + nameMonitor + " missing in JSON provided.");
    Monitor mon;
    mon.componentName = nameMonitor;
    /* For monitors with no detector ID we create dummy IDs starting from -1 and
     * decreasing. */
    mon.detectorID = monitorID--;

    for (const auto &child : children) {
      const auto &val = child[VALUES];
      if (child[NAME] == NAME)
        mon.name = val.asString();
      else if (child[NAME] == DETECTOR_ID || child[NAME] == "detector_number") {
        mon.detectorID = val.asInt();
        /* If there is a detector ID for this monitor increment the dummy IDs to
         * keep them contiguous. */
        ++monitorID;
      } else if (child[NAME] == "events")
        extractMonitorEventStream(child, mon);
      else if (child[NAME] == "waveforms")
        extractMonitorWaveformStream(child, mon);
      else if (validateNXAttribute(child[ATTRIBUTES], NX_TRANSFORMATIONS))
        extractTransformations(child, mon.translation, mon.orientation);
      else if (child[NAME] == SHAPE)
        extractShapeInformation(child, mon.cylinders, mon.faces, mon.vertices, mon.windingOrder, mon.isOffGeometry);
      else if (child[NAME] == DEPENDS_ON)
        verifyDependency(*m_root, child);
    }

    if (validateShapeInformation(mon.isOffGeometry, mon.vertices, mon.cylinders, mon.faces, mon.windingOrder))
      g_log.notice() << "No valid shape information provided for monitor " << mon.componentName << std::endl;

    m_monitors.emplace_back(std::move(mon));
  }
}

/** Extract contents of all choppers found in the instrument and stores
 * information like pslit_edges, top_dead_center stream etc.
 */
void JSONGeometryParser::extractChopperContent() {
  if (m_jsonChoppers.empty())
    return;

  for (const auto &chopper : m_jsonChoppers) {
    const auto &children = (*chopper)[CHILDREN];

    if (children.empty())
      throw std::invalid_argument("Full chopper definition missing in JSON provided.");
    Chopper chop;
    chop.componentName = (*chopper)[NAME].asString();
    for (const auto &child : children) {
      const auto &val = child[VALUES];
      if (child[NAME] == "name")
        chop.name = val.asString();
      else if (child[NAME] == "slit_edges")
        extractDatasetValues(child, chop.slitEdges);
      else if (child[NAME] == "slit_height")
        chop.slitHeight = val.asDouble();
      else if (child[NAME] == "radius")
        chop.radius = val.asDouble();
      else if (child[NAME] == "slits")
        chop.slits = val.asUInt64();
      else if (child[NAME] == "top_dead_center")
        extractChopperTDC(child, chop);
    }

    m_choppers.emplace_back(std::move(chop));
  }
}

/** Parses instrument geometry which is formated in JSON corresponding to the
hdf5 nexus structure.
@param jsonGeometry - JSON string representing the nexus style instrument
@throws std::invalid_argument if the geometry string is invalid.
*/
void JSONGeometryParser::parse(const std::string &jsonGeometry) {
  // Throws when there are issues with the geometry. Performs shallow test for
  // NXEntry, NXSample, NXInstrument and all NXDetector instances.
  validateAndRetrieveGeometry(jsonGeometry);
  extractSampleContent();
  extractSourceContent();
  extractMonitorContent();
  extractChopperContent();
  extractDetectorContent();
}

double JSONGeometryParser::degreesToRadians(const double degrees) noexcept {
  return degrees * M_PI / DEGREES_IN_SEMICIRCLE;
}
} // namespace Mantid::NexusGeometry
