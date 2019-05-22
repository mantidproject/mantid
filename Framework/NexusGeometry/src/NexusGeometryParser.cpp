// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/make_unique.h"
#include "MantidNexusGeometry/Hdf5Version.h"
#include "MantidNexusGeometry/InstrumentBuilder.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidNexusGeometry/TubeHelpers.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <H5Cpp.h>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <numeric>
#include <tuple>
#include <type_traits>

namespace Mantid {
namespace NexusGeometry {

using namespace H5;

// Anonymous namespace
namespace {
const H5G_obj_t GROUP_TYPE = static_cast<H5G_obj_t>(0);
const H5G_obj_t DATASET_TYPE = static_cast<H5G_obj_t>(1);
const H5std_string NX_CLASS = "NX_class";
const H5std_string NX_ENTRY = "NXentry";
const H5std_string NX_INSTRUMENT = "NXinstrument";
const H5std_string NX_DETECTOR = "NXdetector";
const H5std_string NX_MONITOR = "NXmonitor";
const H5std_string NX_SAMPLE = "NXsample";
const H5std_string NX_SOURCE = "NXsource";
const H5std_string DETECTOR_IDS = "detector_number";
const H5std_string DETECTOR_ID = "detector_id";
const H5std_string X_PIXEL_OFFSET = "x_pixel_offset";
const H5std_string Y_PIXEL_OFFSET = "y_pixel_offset";
const H5std_string Z_PIXEL_OFFSET = "z_pixel_offset";
const H5std_string DEPENDS_ON = "depends_on";
const H5std_string NO_DEPENDENCY = ".";
const H5std_string PIXEL_SHAPE = "pixel_shape";
const H5std_string DETECTOR_SHAPE = "detector_shape";
const H5std_string SHAPE = "shape";
// Transformation types
const H5std_string TRANSFORMATION_TYPE = "transformation_type";
const H5std_string TRANSLATION = "translation";
const H5std_string ROTATION = "rotation";
const H5std_string VECTOR = "vector";
const H5std_string UNITS = "units";
// Radians and degrees
const static double PI = 3.1415926535;
const static double DEGREES_IN_SEMICIRCLE = 180;
// Nexus shape types
const H5std_string NX_CYLINDER = "NXcylindrical_geometry";
const H5std_string NX_OFF = "NXoff_geometry";
const H5std_string BANK_NAME = "local_name";

using FaceV = std::vector<Eigen::Vector3d>;

struct Face {
  Eigen::Vector3d v1;
  Eigen::Vector3d v2;
  Eigen::Vector3d v3;
  Eigen::Vector3d v4;
};

bool isDegrees(const H5std_string &units) {
  using boost::regex;
  // Nexus format inexact on acceptable rotation unit definitions
  return regex_match(units, regex("deg(rees)?", regex::icase));
}

template <typename T, typename R>
std::vector<R> convertVector(const std::vector<T> &toConvert) {
  std::vector<R> target(toConvert.size());
  for (size_t i = 0; i < target.size(); ++i) {
    target[i] = R(toConvert[i]);
  }
  return target;
}

template <typename ExpectedT> void validateStorageType(const DataSet &data) {

  const auto typeClass = data.getTypeClass();
  const size_t sizeOfType = data.getDataType().getSize();
  // Early check to prevent reinterpretation of underlying data.
  if (std::is_floating_point<ExpectedT>::value) {
    if (H5T_FLOAT != typeClass) {
      throw std::runtime_error("Storage type mismatch. Expecting to extract a "
                               "floating point number");
    }
    if (sizeOfType != sizeof(ExpectedT)) {
      throw std::runtime_error(
          "Storage type mismatch for floats. This operation "
          "is dangerous. Nexus stored has byte size:" +
          std::to_string(sizeOfType));
    }
  } else if (std::is_integral<ExpectedT>::value) {
    if (H5T_INTEGER != typeClass) {
      throw std::runtime_error(
          "Storage type mismatch. Expecting to extract a integer");
    }
    if (sizeOfType > sizeof(ExpectedT)) {
      // endianness not checked
      throw std::runtime_error(
          "Storage type mismatch for integer. Result "
          "would result in truncation. Nexus stored has byte size:" +
          std::to_string(sizeOfType));
    }
  }
}

template <typename ValueType>
std::vector<ValueType> extractVector(const DataSet &data) {
  validateStorageType<ValueType>(data);
  DataSpace dataSpace = data.getSpace();
  std::vector<ValueType> values;
  values.resize(dataSpace.getSelectNpoints());
  // Read data into vector
  data.read(values.data(), data.getDataType(), dataSpace);
  return values;
}

// Function to read in a dataset into a vector
template <typename ValueType>
std::vector<ValueType> get1DDataset(const H5std_string &dataset,
                                    const H5::Group &group) {
  DataSet data = group.openDataSet(dataset);
  return extractVector<ValueType>(data);
}

// Function to read in a dataset into a vector
template <typename ValueType>
std::vector<ValueType> get1DDataset(const H5File &file,
                                    const H5std_string &dataset) {
  // Open data set
  DataSet data = file.openDataSet(dataset);
  return extractVector<ValueType>(data);
}

std::string get1DStringDataset(const std::string &dataset, const Group &group) {
  // Open data set
  DataSet data = group.openDataSet(dataset);
  auto dataType = data.getDataType();
  // Use a different read method if the string is of variable length type
  if (dataType.isVariableStr()) {
    H5std_string buffer;
    data.read(buffer, dataType, data.getSpace());
    return buffer;
  } else {
    auto nCharacters = dataType.getSize();
    std::vector<char> value(nCharacters);
    data.read(value.data(), dataType, data.getSpace());
    return std::string(value.begin(), value.end());
  }
}

/** Open subgroups of parent group
 *   If firstEntryOnly=true, only the first match is returned as a vector of
 *   size 1.
 */
std::vector<Group> openSubGroups(const Group &parentGroup,
                                 const H5std_string &CLASS_TYPE) {
  std::vector<Group> subGroups;
  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      Group childGroup = parentGroup.openGroup(childPath);
      // Iterate through attributes to find NX_class
      for (uint32_t attribute_index = 0;
           attribute_index < static_cast<uint32_t>(childGroup.getNumAttrs());
           ++attribute_index) {
        // Test attribute at current index for NX_class
        Attribute attribute = childGroup.openAttribute(attribute_index);
        if (attribute.getName() == NX_CLASS) {
          // Get attribute data type
          DataType dataType = attribute.getDataType();
          // Get the NX_class type
          H5std_string classType;
          attribute.read(dataType, classType);
          // If group of correct type, append to subGroup vector
          if (classType == CLASS_TYPE) {
            subGroups.push_back(childGroup);
          }
        }
      }
    }
  }
  return subGroups;
}

/// Find a single dataset inside parent group (returns first match). Unset for
/// no match.
boost::optional<DataSet> findDataset(const Group &parentGroup,
                                     const H5std_string &name) {
  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == DATASET_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      if (childPath == name) {
        auto childDataset = parentGroup.openDataSet(childPath);
        return boost::optional<DataSet>(childDataset);
      }
    }
  }
  return boost::optional<DataSet>{}; // Empty
}
/// Find a single group inside parent (returns first match). class type must
/// match NX_class. Unset for no match.
boost::optional<Group> findGroup(const Group &parentGroup,
                                 const H5std_string &CLASS_TYPE) {
  // Iterate over children, and determine if a group
  for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
    if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
      H5std_string childPath = parentGroup.getObjnameByIdx(i);
      // Open the sub group
      auto childGroup = parentGroup.openGroup(childPath);
      // Iterate through attributes to find NX_class
      for (uint32_t attribute_index = 0;
           attribute_index < static_cast<uint32_t>(childGroup.getNumAttrs());
           ++attribute_index) {
        // Test attribute at current index for NX_class
        Attribute attribute = childGroup.openAttribute(attribute_index);
        if (attribute.getName() == NX_CLASS) {
          // Get attribute data type
          DataType dataType = attribute.getDataType();
          // Get the NX_class type
          H5std_string classType;
          attribute.read(dataType, classType);
          // If group of correct type, return the childGroup
          if (classType == CLASS_TYPE) {
            return boost::optional<Group>(childGroup);
          }
        }
      }
    }
  }
  return boost::optional<Group>{}; // Empty
}

// Get the instrument name
std::string instrumentName(const Group &root) {
  Group entryGroup = *findGroup(root, NX_ENTRY);
  Group instrumentGroup = *findGroup(entryGroup, NX_INSTRUMENT);
  return get1DStringDataset("name", instrumentGroup);
}

// Open all detector groups into a vector
std::vector<Group> openDetectorGroups(const Group &root) {
  std::vector<Group> rawDataGroupPaths = openSubGroups(root, NX_ENTRY);

  // Open all instrument groups within rawDataGroups
  std::vector<Group> instrumentGroupPaths;
  for (auto const &rawDataGroupPath : rawDataGroupPaths) {
    std::vector<Group> instrumentGroups =
        openSubGroups(rawDataGroupPath, NX_INSTRUMENT);
    instrumentGroupPaths.insert(instrumentGroupPaths.end(),
                                instrumentGroups.begin(),
                                instrumentGroups.end());
  }
  // Open all detector groups within instrumentGroups
  std::vector<Group> detectorGroupPaths;
  for (auto const &instrumentGroupPath : instrumentGroupPaths) {
    // Open sub detector groups
    std::vector<Group> detectorGroups =
        openSubGroups(instrumentGroupPath, NX_DETECTOR);
    // Append to detectorGroups vector
    detectorGroupPaths.insert(detectorGroupPaths.end(), detectorGroups.begin(),
                              detectorGroups.end());
  }
  // Return the detector groups
  return detectorGroupPaths;
}

// Function to return the (x,y,z) offsets of pixels in the chosen detectorGroup
Pixels getPixelOffsets(const Group &detectorGroup) {

  // Initialise matrix
  Pixels offsetData;
  std::vector<double> xValues;
  std::vector<double> yValues;
  std::vector<double> zValues;
  for (unsigned int i = 0; i < detectorGroup.getNumObjs(); i++) {
    H5std_string objName = detectorGroup.getObjnameByIdx(i);
    if (objName == X_PIXEL_OFFSET) {
      xValues = get1DDataset<double>(objName, detectorGroup);
    }
    if (objName == Y_PIXEL_OFFSET) {
      yValues = get1DDataset<double>(objName, detectorGroup);
    }
    if (objName == Z_PIXEL_OFFSET) {
      zValues = get1DDataset<double>(objName, detectorGroup);
    }
  }

  // Determine size of dataset
  int rowLength = 0;
  bool xEmpty = xValues.empty();
  bool yEmpty = yValues.empty();
  bool zEmpty = zValues.empty();

  if (!xEmpty)
    rowLength = static_cast<int>(xValues.size());
  else if (!yEmpty)
    rowLength = static_cast<int>(yValues.size());
  // Need at least 2 dimensions to define points
  else
    return offsetData;

  // Default x,y,z to zero if no data provided
  offsetData.resize(3, rowLength);
  offsetData.setZero(3, rowLength);

  if (!xEmpty) {
    for (int i = 0; i < rowLength; i++)
      offsetData(0, i) = xValues[i];
  }
  if (!yEmpty) {
    for (int i = 0; i < rowLength; i++)
      offsetData(1, i) = yValues[i];
  }
  if (!zEmpty) {
    for (int i = 0; i < rowLength; i++)
      offsetData(2, i) = zValues[i];
  }
  // Return the coordinate matrix
  return offsetData;
}

/**
 * Creates a Homogeneous transfomation for nexus groups
 *
 * Walks the chain of transformations described in the file where W1 is first
 *transformation and Wn is last and assembles them as
 *
 * W = Wn x ... W2 x W1
 *
 * Each W describes a Homogenous Transformation
 *
 * R | T
 * -   -
 * 0 | 1
 *
 *
 * @param file
 * @param detectorGroup
 * @return
 */
Eigen::Transform<double, 3, Eigen::Affine>
getTransformations(const H5File &file, const Group &detectorGroup) {
  H5std_string dependency;
  // Get absolute dependency path
  auto status =
      H5Gget_objinfo(detectorGroup.getId(), DEPENDS_ON.c_str(), false, nullptr);
  if (status == 0) {
    dependency = get1DStringDataset(DEPENDS_ON, detectorGroup);
  } else {
    return Eigen::Transform<double, 3, Eigen::Affine>::Identity();
  }

  // Initialise transformation holder as identity matrix
  auto transforms = Eigen::Transform<double, 3, Eigen::Affine>::Identity();
  // Breaks when no more dependencies (dependency = ".")
  // Transformations must be applied in the order of direction of discovery
  // (they are _passive_ transformations)
  while (dependency != NO_DEPENDENCY) {
    // Open the transformation data set
    DataSet transformation = file.openDataSet(dependency);

    // Get magnitude of current transformation
    double magnitude = get1DDataset<double>(file, dependency)[0];
    // Containers for transformation data
    Eigen::Vector3d transformVector(0.0, 0.0, 0.0);
    H5std_string transformType;
    H5std_string transformUnits;
    for (uint32_t i = 0;
         i < static_cast<uint32_t>(transformation.getNumAttrs()); i++) {
      // Open attribute at current index
      Attribute attribute = transformation.openAttribute(i);
      H5std_string attributeName = attribute.getName();
      // Get next dependency
      if (attributeName == DEPENDS_ON) {
        DataType dataType = attribute.getDataType();
        attribute.read(dataType, dependency);
      }
      // Get transform type
      else if (attributeName == TRANSFORMATION_TYPE) {
        DataType dataType = attribute.getDataType();
        attribute.read(dataType, transformType);
      }
      // Get unit vector for transformation
      else if (attributeName == VECTOR) {
        std::vector<double> unitVector;
        DataType dataType = attribute.getDataType();

        // Get data size
        DataSpace dataSpace = attribute.getSpace();

        // Resize vector to hold data
        unitVector.resize(dataSpace.getSelectNpoints());

        // Read the data into Eigen vector
        attribute.read(dataType, unitVector.data());
        transformVector(0) = unitVector[0];
        transformVector(1) = unitVector[1];
        transformVector(2) = unitVector[2];
      } else if (attributeName == UNITS) {
        DataType dataType = attribute.getDataType();
        attribute.read(dataType, transformUnits);
      }
    }
    // Transform_type = translation
    if (transformType == TRANSLATION) {
      // Translation = magnitude*unitVector
      transformVector *= magnitude;
      Eigen::Translation3d translation(transformVector);
      transforms = translation * transforms;
    } else if (transformType == ROTATION) {
      double angle = magnitude;
      if (isDegrees(transformUnits)) {
        // Convert angle from degrees to radians
        angle *= PI / DEGREES_IN_SEMICIRCLE;
      }
      Eigen::AngleAxisd rotation(angle, transformVector);
      transforms = rotation * transforms;
    } else {
      throw std::runtime_error(
          "Unknown Transform type in Nexus Geometry Parsing");
    }
  }
  return transforms;
}

// Function to return the detector ids in the same order as the offsets
std::vector<Mantid::detid_t> getDetectorIds(const Group &detectorGroup) {

  std::vector<Mantid::detid_t> detIds;
  if (!findDataset(detectorGroup, DETECTOR_IDS))
    throw std::invalid_argument("Mantid requires the following named dataset "
                                "to be present in NXDetectors: " +
                                DETECTOR_IDS);
  for (unsigned int i = 0; i < detectorGroup.getNumObjs(); ++i) {
    H5std_string objName = detectorGroup.getObjnameByIdx(i);
    if (objName == DETECTOR_IDS) {
      const auto data = detectorGroup.openDataSet(objName);
      if (data.getDataType().getSize() == 8) {
        // Note the narrowing here!
        detIds = convertVector<int64_t, Mantid::detid_t>(
            extractVector<int64_t>(data));
      } else {
        detIds = extractVector<Mantid::detid_t>(data);
      }
    }
  }
  return detIds;
}

// Parse cylinder nexus geometry
boost::shared_ptr<const Geometry::IObject>
parseNexusCylinder(const Group &shapeGroup) {
  H5std_string pointsToVertices = "cylinders";
  std::vector<int> cPoints = get1DDataset<int>(pointsToVertices, shapeGroup);

  H5std_string verticesData = "vertices";
  // 1D reads row first, then columns
  std::vector<double> vPoints = get1DDataset<double>(verticesData, shapeGroup);
  Eigen::Map<Eigen::Matrix<double, 3, 3>> vertices(vPoints.data());
  // Read points into matrix, sorted by cPoints ordering
  Eigen::Matrix<double, 3, 3> vSorted;
  for (int i = 0; i < 3; ++i) {
    vSorted.col(cPoints[i]) = vertices.col(i);
  }
  return NexusShapeFactory::createCylinder(vSorted);
}

// Parse OFF (mesh) nexus geometry
boost::shared_ptr<const Geometry::IObject>
parseNexusMesh(const Group &shapeGroup) {
  const std::vector<uint32_t> faceIndices = convertVector<int32_t, uint32_t>(
      get1DDataset<int32_t>("faces", shapeGroup));
  const std::vector<uint32_t> windingOrder = convertVector<int32_t, uint32_t>(
      get1DDataset<int32_t>("winding_order", shapeGroup));
  const auto vertices = get1DDataset<float>("vertices", shapeGroup);
  return NexusShapeFactory::createFromOFFMesh(faceIndices, windingOrder,
                                              vertices);
}

void extractFacesAndIDs(const std::vector<uint32_t> &detFaces,
                        const std::vector<uint32_t> &windingOrder,
                        const std::vector<float> &vertices,
                        const std::unordered_map<int, uint32_t> &detIdToIndex,
                        const size_t vertsPerFace,
                        std::vector<std::vector<Eigen::Vector3d>> &detFaceVerts,
                        std::vector<std::vector<uint32_t>> &detFaceIndices,
                        std::vector<std::vector<uint32_t>> &detWindingOrder,
                        std::vector<int32_t> &detIds) {
  const size_t vertStride = 3;
  size_t detFaceIndex = 1;
  std::fill(detFaceIndices.begin(), detFaceIndices.end(),
            std::vector<uint32_t>(1, 0));
  for (size_t i = 0; i < windingOrder.size(); i += vertsPerFace) {
    auto detFaceId = detFaces[detFaceIndex];
    // Id -> Index
    auto detIndex = detIdToIndex.at(detFaceId);
    auto &detVerts = detFaceVerts[detIndex];
    auto &detIndices = detFaceIndices[detIndex];
    auto &detWinding = detWindingOrder[detIndex];
    detVerts.reserve(vertsPerFace);
    detWinding.reserve(vertsPerFace);
    for (size_t v = 0; v < vertsPerFace; ++v) {
      const auto vi = windingOrder[i + v] * vertStride;
      detVerts.emplace_back(vertices[vi], vertices[vi + 1], vertices[vi + 2]);
      detWinding.push_back(static_cast<uint32_t>(detWinding.size()));
    }
    // Index -> Id
    detIds[detIndex] = detFaceId;
    detIndices.push_back(static_cast<uint32_t>(detVerts.size()));
    // Detector faces is 2N detectors
    detFaceIndex += 2;
  }
}

void parseNexusMeshAndAddDetectors(
    const std::vector<uint32_t> &detFaces,
    const std::vector<uint32_t> &faceIndices,
    const std::vector<uint32_t> &windingOrder,
    const std::vector<float> &vertices, const size_t numDets,
    const std::unordered_map<int, uint32_t> &detIdToIndex,
    const std::string &name, InstrumentBuilder &builder) {
  auto vertsPerFace = windingOrder.size() / faceIndices.size();
  std::vector<std::vector<Eigen::Vector3d>> detFaceVerts(numDets);
  std::vector<std::vector<uint32_t>> detFaceIndices(numDets);
  std::vector<std::vector<uint32_t>> detWindingOrder(numDets);
  std::vector<int> detIds(numDets);

  extractFacesAndIDs(detFaces, windingOrder, vertices, detIdToIndex,
                     vertsPerFace, detFaceVerts, detFaceIndices,
                     detWindingOrder, detIds);

  for (size_t i = 0; i < numDets; ++i) {
    auto &detVerts = detFaceVerts[i];
    const auto &detIndices = detFaceIndices[i];
    const auto &detWinding = detWindingOrder[i];
    // Calculate polygon centre
    auto centre = std::accumulate(detVerts.begin() + 1, detVerts.end(),
                                  detVerts.front()) /
                  detVerts.size();

    // translate shape to origin for shape coordinates.
    std::for_each(detVerts.begin(), detVerts.end(),
                  [&centre](Eigen::Vector3d &val) { val -= centre; });

    auto shape =
        NexusShapeFactory::createFromOFFMesh(detIndices, detWinding, detVerts);
    builder.addDetectorToLastBank(name + "_" + std::to_string(i), detIds[i],
                                  centre, std::move(shape));
  }
}

void parseAndAddBank(const Group &shapeGroup, InstrumentBuilder &builder,
                     const std::vector<int> &detectorIds,
                     const std::string &bankName) {
  // Load mapping between detector IDs and faces, winding order of vertices for
  // faces, and face corner vertices.
  const std::vector<uint32_t> detFaces = convertVector<int32_t, uint32_t>(
      get1DDataset<int32_t>("detector_faces", shapeGroup));
  const std::vector<uint32_t> faceIndices = convertVector<int32_t, uint32_t>(
      get1DDataset<int32_t>("faces", shapeGroup));
  const std::vector<uint32_t> windingOrder = convertVector<int32_t, uint32_t>(
      get1DDataset<int32_t>("winding_order", shapeGroup));
  const auto vertices = get1DDataset<float>("vertices", shapeGroup);

  // Build a map of detector IDs to the index of occurrence in the
  // "detector_number" dataset
  std::unordered_map<int, uint32_t> detIdToIndex;
  for (uint32_t i = 0; i < detectorIds.size(); ++i) {
    detIdToIndex.emplace(detectorIds[i], i);
  }

  parseNexusMeshAndAddDetectors(detFaces, faceIndices, windingOrder, vertices,
                                detectorIds.size(), detIdToIndex, bankName,
                                builder);
}

/// Choose what shape type to parse
boost::shared_ptr<const Geometry::IObject>
parseNexusShape(const Group &detectorGroup, bool &searchTubes) {
  bool isGroup = false;
  Group shapeGroup;
  try {
    shapeGroup = detectorGroup.openGroup(PIXEL_SHAPE);
    isGroup = true;
  } catch (H5::Exception &) {
    // TODO. Current assumption. Can we have pixels without specifying a
    // shape?
    try {
      shapeGroup = detectorGroup.openGroup(SHAPE);
    } catch (H5::Exception &) {
      return boost::shared_ptr<const Geometry::IObject>(nullptr);
    }
  }

  H5std_string shapeType;
  for (uint32_t i = 0; i < static_cast<uint32_t>(shapeGroup.getNumAttrs());
       ++i) {
    Attribute attribute = shapeGroup.openAttribute(i);
    H5std_string attributeName = attribute.getName();
    if (attributeName == NX_CLASS) {
      attribute.read(attribute.getDataType(), shapeType);
    }
  }
  // Give shape group to correct shape parser
  if (shapeType == NX_CYLINDER) {
    searchTubes = isGroup;
    return parseNexusCylinder(shapeGroup);
  } else if (shapeType == NX_OFF) {
    return parseNexusMesh(shapeGroup);
  } else {
    throw std::runtime_error(
        "Shape type not recognised by NexusGeometryParser");
  }
}

// Parse source and add to instrument
void parseAndAddSource(const H5File &file, const Group &root,
                       InstrumentBuilder &builder) {
  Group entryGroup = *findGroup(root, NX_ENTRY);
  Group instrumentGroup = *findGroup(entryGroup, NX_INSTRUMENT);
  Group sourceGroup = *findGroup(instrumentGroup, NX_SOURCE);
  std::string sourceName = "Unspecfied";
  if (findDataset(sourceGroup, "name"))
    sourceName = get1DStringDataset("name", sourceGroup);
  auto sourceTransformations = getTransformations(file, sourceGroup);
  auto defaultPos = Eigen::Vector3d(0.0, 0.0, 0.0);
  builder.addSource(sourceName, sourceTransformations * defaultPos);
}

// Parse sample and add to instrument
void parseAndAddSample(const H5File &file, const Group &root,
                       InstrumentBuilder &builder) {
  Group entryGroup = *findGroup(root, NX_ENTRY);
  Group sampleGroup = *findGroup(entryGroup, NX_SAMPLE);
  auto sampleTransforms = getTransformations(file, sampleGroup);
  Eigen::Vector3d samplePos = sampleTransforms * Eigen::Vector3d(0.0, 0.0, 0.0);
  std::string sampleName = "Unspecified";
  if (findDataset(sampleGroup, "name"))
    sampleName = get1DStringDataset("name", sampleGroup);
  builder.addSample(sampleName, samplePos);
}

void parseMonitors(const H5File &file, const H5::Group &root,
                   InstrumentBuilder &builder) {
  std::vector<Group> rawDataGroupPaths = openSubGroups(root, NX_ENTRY);

  // Open all instrument groups within rawDataGroups
  for (auto const &rawDataGroupPath : rawDataGroupPaths) {
    std::vector<Group> instrumentGroups =
        openSubGroups(rawDataGroupPath, NX_INSTRUMENT);
    for (auto &inst : instrumentGroups) {
      std::vector<Group> monitorGroups = openSubGroups(inst, NX_MONITOR);
      for (auto &monitor : monitorGroups) {
        if (!findDataset(monitor, DETECTOR_ID))
          throw std::invalid_argument("NXmonitors must have " + DETECTOR_ID);
        auto detectorId = get1DDataset<int64_t>(DETECTOR_ID, monitor)[0];
        bool proxy = false;
        auto monitorShape = parseNexusShape(monitor, proxy);
        auto monitorTransforms = getTransformations(file, monitor);
        builder.addMonitor(
            std::to_string(detectorId), static_cast<int32_t>(detectorId),
            monitorTransforms * Eigen::Vector3d{0, 0, 0}, monitorShape);
      }
    }
  }
}

std::unique_ptr<const Mantid::Geometry::Instrument>
extractInstrument(const H5File &file, const Group &root) {
  InstrumentBuilder builder(instrumentName(root));
  // Get path to all detector groups
  const std::vector<Group> detectorGroups = openDetectorGroups(root);
  for (auto &detectorGroup : detectorGroups) {
    // Transform in homogenous coordinates. Offsets will be rotated then bank
    // translation applied.
    Eigen::Transform<double, 3, 2> transforms =
        getTransformations(file, detectorGroup);
    // Absolute bank position
    Eigen::Vector3d bankPos = transforms * Eigen::Vector3d{0, 0, 0};
    // Absolute bank rotation
    auto bankRotation = Eigen::Quaterniond(transforms.rotation());
    std::string bankName;
    if (findDataset(detectorGroup, BANK_NAME))
      bankName = get1DStringDataset(BANK_NAME,
                                    detectorGroup); // local_name is optional
    builder.addBank(bankName, bankPos, bankRotation);
    // Get the pixel detIds
    auto detectorIds = getDetectorIds(detectorGroup);

    try {
      auto shapeGroup = detectorGroup.openGroup(DETECTOR_SHAPE);
      parseAndAddBank(shapeGroup, builder, detectorIds, bankName);
      continue;
    } catch (H5::Exception &) { // No detector_shape group
    }

    // Get the pixel offsets
    Pixels pixelOffsets = getPixelOffsets(detectorGroup);
    // Calculate pixel relative positions
    Pixels detectorPixels = Eigen::Affine3d::Identity() * pixelOffsets;
    bool searchTubes = false;
    // Extract shape
    auto detShape = parseNexusShape(detectorGroup, searchTubes);

    if (searchTubes) {
      auto tubes =
          TubeHelpers::findAndSortTubes(*detShape, detectorPixels, detectorIds);
      builder.addTubes(bankName, tubes, detShape);
    } else {
      for (size_t i = 0; i < detectorIds.size(); ++i) {
        auto index = static_cast<int>(i);
        std::string name = bankName + "_" + std::to_string(index);

        Eigen::Vector3d relativePos = detectorPixels.col(index);
        builder.addDetectorToLastBank(name, detectorIds[index], relativePos,
                                      detShape);
      }
    }
  }
  // Sort the detectors
  // Parse source and sample and add to instrument
  parseAndAddSample(file, root, builder);
  parseAndAddSource(file, root, builder);
  parseMonitors(file, root, builder);
  return builder.createInstrument();
}
} // namespace

std::unique_ptr<const Geometry::Instrument>
NexusGeometryParser::createInstrument(const std::string &fileName) {

  const H5File file(fileName, H5F_ACC_RDONLY);
  auto rootGroup = file.openGroup("/");
  return extractInstrument(file, rootGroup);
}

// Create a unique instrument name from Nexus file
std::string NexusGeometryParser::getMangledName(const std::string &fileName,
                                                const std::string &instName) {
  std::string mangledName = instName;
  if (!fileName.empty()) {
    std::string checksum =
        Mantid::Kernel::ChecksumHelper::sha1FromString(fileName);
    mangledName += checksum;
  }
  return mangledName;
}
} // namespace NexusGeometry
} // namespace Mantid
