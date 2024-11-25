// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidNexusGeometry/AbstractLogger.h"
#include "MantidNexusGeometry/H5ForwardCompatibility.h"
#include "MantidNexusGeometry/Hdf5Version.h"
#include "MantidNexusGeometry/InstrumentBuilder.h"
#include "MantidNexusGeometry/NexusGeometryDefinitions.h"
#include "MantidNexusGeometry/NexusGeometryUtilities.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidNexusGeometry/TubeHelpers.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <H5Cpp.h>
#include <boost/regex.hpp>
#include <numeric>
#include <sstream>
#include <tuple>
#include <type_traits>

namespace Mantid::NexusGeometry {
using namespace H5;

// Anonymous namespace
namespace {

bool isDegrees(const H5std_string &units) {
  using boost::regex;
  // Nexus format inexact on acceptable rotation unit definitions
  return regex_match(units, regex("deg(rees)?", regex::icase));
}

template <typename T, typename R> std::vector<R> convertVector(const std::vector<T> &toConvert) {
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
                               "floating point number from " +
                               H5_OBJ_NAME(data));
    }
    if (sizeOfType != sizeof(ExpectedT)) {
      throw std::runtime_error("Storage type mismatch for floats. This operation "
                               "is dangerous. Nexus stored has byte size:" +
                               std::to_string(sizeOfType) + " in " + H5_OBJ_NAME(data));
    }
  } else if (std::is_integral<ExpectedT>::value) {
    if (H5T_INTEGER != typeClass) {
      throw std::runtime_error("Storage type mismatch. Expecting to extract a integer from " + H5_OBJ_NAME(data));
    }
    if (sizeOfType > sizeof(ExpectedT)) {
      // endianness not checked
      throw std::runtime_error("Storage type mismatch for integer. Result "
                               "would result in truncation. Nexus stored has byte size:" +
                               std::to_string(sizeOfType) + " in " + H5_OBJ_NAME(data));
    }
  }
}

template <typename ValueType> std::vector<ValueType> extractVector(const DataSet &data) {
  validateStorageType<ValueType>(data);
  DataSpace dataSpace = data.getSpace();
  std::vector<ValueType> values;
  values.resize(dataSpace.getSelectNpoints());
  // Read data into vector
  data.read(values.data(), data.getDataType(), dataSpace);
  return values;
}

/**
 * Parser as local class. Makes logging (side-effect) easier.
 */
class Parser {
private:
  // Logger object
  std::unique_ptr<AbstractLogger> m_logger;

  /**
   * The function allows us to determine where problems are and logs key
   * information.
   */
  template <typename T> DataSet openDataSet(T host, const std::string &name) {
    DataSet ret;
    try {
      ret = host.openDataSet(name);
    } catch (H5::Exception &ex) {
      // Capture key information
      m_logger->error(ex.getFuncName());
      m_logger->error(ex.getDetailMsg());
      throw;
    }
    return ret;
  }

  // Function to read in a dataset into a vector. Prefer not call directly, use
  // readNX.. functions instead
  template <typename ValueType, typename T>
  std::vector<ValueType> get1DDataset(const T &host, const H5std_string &dataset) {
    DataSet data = openDataSet(host, dataset);
    return extractVector<ValueType>(data);
  }

  // Read NXInts - provides abstraction for reading different sized integers
  // arrays http://download.nexusformat.org/doc/html/nxdl-types.html#nx-int
  template <typename T> std::vector<int64_t> readNXInts(const T &object, const std::string &dsName) {
    std::vector<int64_t> ints;
    const auto nxintsize = object.openDataSet(dsName).getDataType().getSize();
    if (nxintsize == sizeof(int32_t)) {
      ints = convertVector<int32_t, int64_t>(get1DDataset<int32_t>(object, dsName));
    } else if (nxintsize == sizeof(int64_t)) {
      ints = get1DDataset<int64_t>(object, dsName);
    } else {
      std::stringstream ss;
      ss << "Cannot handle reading ints of size " << nxintsize << " from " << dsName << " in " << H5_OBJ_NAME(object)
         << ". Only 64 and 32 bit signed ints handled";
      throw std::runtime_error(ss.str());
    }

    return ints;
  }
  // Read NXInts and cast to Uint32 expecting datasets to be stored as arrays
  // http://download.nexusformat.org/doc/html/nxdl-types.html#nx-int rather than
  // http://download.nexusformat.org/doc/html/nxdl-types.html#nx-uint
  template <typename T> std::vector<uint32_t> readNXUInts32(const T &object, const std::string &dsName) {
    std::vector<uint32_t> ints;
    const auto nxintsize = object.openDataSet(dsName).getDataType().getSize();
    if (nxintsize == sizeof(int32_t)) {
      ints = convertVector<int32_t, uint32_t>(get1DDataset<int32_t>(object, dsName));
    } else if (nxintsize == sizeof(int64_t)) {
      ints = convertVector<int64_t, uint32_t>(get1DDataset<int64_t>(object, dsName));
    } else {
      std::stringstream ss;
      ss << "Cannot handle reading ints of size " << nxintsize << " from " << dsName << " in " << H5_OBJ_NAME(object)
         << ". Only 64 and 32 bit signed ints handled";
      throw std::runtime_error(ss.str());
    }
    return ints;
  }

  template <typename T>
  typename std::enable_if<std::is_base_of<H5::H5Object, T>::value, std::string>::type
  unsupportedNXFloatMessage(size_t nxfloatsize, const T &object, const std::string &dsName) {
    std::stringstream ss;
    ss << "Cannot handle reading ints of size " << nxfloatsize << " from " << dsName << " in " << H5_OBJ_NAME(object)
       << ". Only 64 and 32 bit floats handled";
    return ss.str();
  }
  template <typename T>
  typename std::enable_if<!std::is_base_of<H5::H5Object, T>::value, std::string>::type
  unsupportedNXFloatMessage(size_t nxfloatsize, const T &, const std::string &dsName) {
    std::stringstream ss;
    ss << "Cannot handle reading ints of size " << nxfloatsize << " from " << dsName
       << ". Only 64 and 32 bit floats handled";
    return ss.str();
  }

  // Read NXFloats - provides abstraction for reading different sized integers
  // arrays http://download.nexusformat.org/doc/html/nxdl-types.html#nx-float
  template <typename T> std::vector<double> readNXFloats(const T &object, const std::string &dsName) {
    std::vector<double> floats;
    const auto nxfloatsize = object.openDataSet(dsName).getDataType().getSize();
    if (nxfloatsize == sizeof(float_t)) {
      floats = convertVector<float_t, double_t>(get1DDataset<float_t>(object, dsName));
    } else if (nxfloatsize == sizeof(double_t)) {
      floats = get1DDataset<double_t>(object, dsName);
    } else {
      throw std::runtime_error(unsupportedNXFloatMessage(nxfloatsize, object, dsName));
    }
    return floats;
  }

  std::string get1DStringDataset(const std::string &dataset, const Group &group) {
    // Open data set
    DataSet data = openDataSet(group, dataset);
    auto dataType = data.getDataType();
    // Use a different read method if the string is of variable length type
    if (dataType.isVariableStr()) {
      H5std_string buffer;
      // Need to check for old versions of hdf5
      if (Hdf5Version::checkVariableLengthStringSupport()) {
        data.read(buffer, dataType, data.getSpace());
      } else {
        m_logger->warning("NexusGeometryParser::get1DStringDataset: Only "
                          "versions 1.8.16 + of hdf5 support the variable "
                          "string feature. This could be terminal.");
      }
      return buffer;
    } else {
      auto nCharacters = dataType.getSize();
      std::vector<char> value(nCharacters);
      data.read(value.data(), dataType, data.getSpace());
      auto str = std::string(value.begin(), value.end());
      str.erase(std::find(str.begin(), str.end(), '\0'), str.end());
      return str;
    }
  }

  // Provided to support invalid or empty null-termination character strings
  std::string readOrSubstitute(const std::string &dataset, const Group &group, const std::string &substitute) {
    auto read = get1DStringDataset(dataset, group);
    if (read.empty())
      read = substitute;
    return read;
  }

  /** Open subgroups of parent group
   *   If firstEntryOnly=true, only the first match is returned as a vector of
   *   size 1.
   */
  std::vector<Group> openSubGroups(const Group &parentGroup, const H5std_string &CLASS_TYPE) {
    std::vector<Group> subGroups;
    // Iterate over children, and determine if a group
    for (hsize_t i = 0; i < parentGroup.getNumObjs(); ++i) {
      if (parentGroup.getObjTypeByIdx(i) == GROUP_TYPE) {
        H5std_string childPath = parentGroup.getObjnameByIdx(i);
        // Open the sub group
        Group childGroup = parentGroup.openGroup(childPath);
        // Iterate through attributes to find NX_class
        for (uint32_t attribute_index = 0; attribute_index < static_cast<uint32_t>(childGroup.getNumAttrs());
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
              subGroups.emplace_back(childGroup);
            }
          }
        }
      }
    }
    return subGroups;
  }

  // Get the instrument name
  std::string instrumentName(const Group &parent) {
    Group instrumentGroup = *utilities::findGroup(parent, NX_INSTRUMENT);
    return get1DStringDataset("name", instrumentGroup);
  }

  // Open all detector subgroups into a vector
  std::vector<Group> openDetectorGroups(const Group &parent) {

    // This method was originally written to open all detector groups for all instruments in the file.
    // But then it was used only with files containing a single instrument.
    //   In order to work correctly for files containing multiple workspaces,
    // this method needs to open _only_ the detector groups for the current instrument.

    Group instrumentGroup = *utilities::findGroup(parent, NX_INSTRUMENT);

    // Open all detector subgroups within the instrument
    std::vector<Group> detectorGroups = openSubGroups(instrumentGroup, NX_DETECTOR);

    // Return the detector groups
    return detectorGroups;
  }

  // Function to return the (x,y,z) offsets of pixels in the chosen
  // detectorGroup
  Pixels getPixelOffsets(const Group &detectorGroup) {

    // Initialise matrix
    Pixels offsetData;
    std::vector<double> xValues;
    std::vector<double> yValues;
    std::vector<double> zValues;
    for (unsigned int i = 0; i < detectorGroup.getNumObjs(); i++) {
      H5std_string objName = detectorGroup.getObjnameByIdx(i);
      if (objName == X_PIXEL_OFFSET) {
        xValues = readNXFloats(detectorGroup, objName);
      }
      if (objName == Y_PIXEL_OFFSET) {
        yValues = readNXFloats(detectorGroup, objName);
      }
      if (objName == Z_PIXEL_OFFSET) {
        zValues = readNXFloats(detectorGroup, objName);
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
    else if (!zEmpty)
      rowLength = static_cast<int>(zValues.size());
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
   * Creates a Homogeneous transformation for NeXus groups
   *
   * Walks the chain of transformations described in the file where W1 is first
   * transformation and Wn is last and assembles them as
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
  Eigen::Transform<double, 3, Eigen::Affine> getTransformations(const H5File &file, const Group &detectorGroup) {
    H5std_string dependency;
    // Get absolute dependency path
    auto status = H5Gget_objinfo(detectorGroup.getId(), DEPENDS_ON.c_str(), false, nullptr);
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
      DataSet transformation = openDataSet(file, dependency);

      // Get magnitude of current transformation
      double magnitude = readNXFloats(file, dependency)[0];
      // Containers for transformation data
      Eigen::Vector3d transformVector(0.0, 0.0, 0.0);
      H5std_string transformType;
      H5std_string transformUnits;
      for (uint32_t i = 0; i < static_cast<uint32_t>(transformation.getNumAttrs()); i++) {
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
          angle *= M_PI / DEGREES_IN_SEMICIRCLE;
        }
        Eigen::AngleAxisd rotation(angle, transformVector);
        transforms = rotation * transforms;
      } else {
        throw std::runtime_error("Unknown Transform type \"" + transformType + "\" found in " +
                                 H5_OBJ_NAME(transformation) + " when parsing Nexus geometry");
      }
    }
    return transforms;
  }

  // Function to return the detector ids in the same order as the offsets
  std::vector<Mantid::detid_t> getDetectorIds(const Group &detectorGroup) {

    std::vector<Mantid::detid_t> detIds;
    if (!utilities::findDataset(detectorGroup, DETECTOR_IDS))
      throw std::invalid_argument("Mantid requires the following named dataset "
                                  "to be present in NXDetectors: " +
                                  DETECTOR_IDS);
    for (unsigned int i = 0; i < detectorGroup.getNumObjs(); ++i) {
      H5std_string objName = detectorGroup.getObjnameByIdx(i);
      if (objName == DETECTOR_IDS) {
        const auto data = openDataSet(detectorGroup, objName);
        if (data.getDataType().getSize() == sizeof(int64_t)) {
          // Note the narrowing here!
          detIds = convertVector<int64_t, Mantid::detid_t>(extractVector<int64_t>(data));
        } else {
          detIds = extractVector<Mantid::detid_t>(data);
        }
      }
    }
    return detIds;
  }

  // Parse cylinder nexus geometry
  void parseNexusCylinderDetector(const Group &shapeGroup, const std::string &name, InstrumentBuilder &builder,
                                  const std::vector<Mantid::detid_t> &detectorIds) {

    const auto cylinderIndexToDetId = getDetectorIds(shapeGroup); // 2x detids size
    const auto cPoints = readNXInts(shapeGroup, "cylinders");
    // 1D reads row first, then columns
    auto vPoints = readNXFloats(shapeGroup, "vertices");
    if (cylinderIndexToDetId.size() != 2 * detectorIds.size())
      throw std::runtime_error("numbers of detector with shape cylinder does "
                               "not match number of detectors");
    if (cPoints.size() % 3 != 0)
      throw std::runtime_error("cylinders not divisible by 3. Bad input.");
    if (vPoints.size() % 3 != 0)
      throw std::runtime_error("vertices not divisible by 3. Bad input.");

    for (size_t i = 0; i < cylinderIndexToDetId.size(); i += 2) {
      auto cylinderIndex = cylinderIndexToDetId[i];
      auto detId = cylinderIndexToDetId[i + 1];

      Eigen::Matrix<double, 3, 3> vSorted;
      for (uint8_t j = 0; j < 3; ++j) {
        auto vertexIndex = cPoints[cylinderIndex * 3 + j] * 3;
        vSorted(j * 3) = vPoints[vertexIndex];
        vSorted(j * 3 + 1) = vPoints[vertexIndex + 1];
        vSorted(j * 3 + 2) = vPoints[vertexIndex + 2];
      }
      const auto centre = vSorted.col(0);
      const auto other = vSorted.col(2);

      // Note that tube optimisation is not used here. That should be applied as
      // future optimisation.
      builder.addDetectorToLastBank(name + "_" + std::to_string(cylinderIndex), detId, (centre + other) / 2,
                                    NexusShapeFactory::createCylinder(vSorted));
    }
  }

  std::shared_ptr<const Geometry::IObject> parseNexusCylinder(const Group &shapeGroup) {
    H5std_string pointsToVertices = "cylinders";
    auto cPoints = readNXInts(shapeGroup, pointsToVertices);

    H5std_string verticesData = "vertices";
    // 1D reads row first, then columns
    auto vPoints = readNXFloats(shapeGroup, verticesData);
    Eigen::Map<Eigen::Matrix<double, 3, 3>> vertices(vPoints.data());
    // Read points into matrix, sorted by cPoints ordering
    Eigen::Matrix<double, 3, 3> vSorted;
    for (int i = 0; i < 3; ++i) {
      vSorted.col(cPoints[i]) = vertices.col(i);
    }
    return NexusShapeFactory::createCylinder(vSorted);
  }

  // Parse OFF (mesh) nexus geometry
  std::shared_ptr<const Geometry::IObject> parseNexusMesh(const Group &shapeGroup) {
    const auto faceIndices = readNXUInts32(shapeGroup, "faces");
    const auto windingOrder = readNXUInts32(shapeGroup, "winding_order");
    const auto vertices = readNXFloats(shapeGroup, "vertices");
    return NexusShapeFactory::createFromOFFMesh(faceIndices, windingOrder, vertices);
  }

  void extractFacesAndIDs(const std::vector<uint32_t> &detFaces, const std::vector<uint32_t> &windingOrder,
                          const std::vector<double> &vertices, const std::unordered_map<int, uint32_t> &detIdToIndex,
                          const std::vector<uint32_t> &faceIndices,
                          std::vector<std::vector<Eigen::Vector3d>> &detFaceVerts,
                          std::vector<std::vector<uint32_t>> &detFaceIndices,
                          std::vector<std::vector<uint32_t>> &detWindingOrder, std::vector<int32_t> &detIds) {
    for (size_t i = 0; i < detFaces.size(); i += 2) {
      const auto faceIndexOfDetector = detFaces[i];
      const auto faceIndex = faceIndices[faceIndexOfDetector];
      auto nextFaceIndex = windingOrder.size();
      if (faceIndexOfDetector + 1 < faceIndices.size())
        nextFaceIndex = faceIndices[faceIndexOfDetector + 1];
      const auto nVertsInFace = nextFaceIndex - faceIndex;
      const auto detID = detFaces[i + 1];
      const auto detIndex = detIdToIndex.at(detID);
      auto &vertsForDet = detFaceVerts[detIndex];
      auto &detWinding = detWindingOrder[detIndex];
      vertsForDet.reserve(nVertsInFace);
      detWinding.reserve(nVertsInFace);
      detFaceIndices[detIndex].emplace_back(faceIndex); // Associate face with detector index
                                                        // Use face index to index into winding order.
      for (size_t v = 0; v < nVertsInFace; ++v) {
        const auto vi = windingOrder[faceIndex + v] * 3;
        vertsForDet.emplace_back(vertices[vi], vertices[vi + 1], vertices[vi + 2]);
        detWinding.emplace_back(static_cast<uint32_t>(detWinding.size()));
      }
      // Index -> Id
      detIds[detIndex] = detID;
    }
  }

  void extractNexusMeshAndAddDetectors(const std::vector<uint32_t> &detFaces, const std::vector<uint32_t> &faceIndices,
                                       const std::vector<uint32_t> &windingOrder, const std::vector<double> &vertices,
                                       const size_t numDets, const std::unordered_map<int, uint32_t> &detIdToIndex,
                                       const std::string &name, InstrumentBuilder &builder,
                                       const Group &detectorGroup) {
    std::vector<std::vector<Eigen::Vector3d>> detFaceVerts(numDets);
    std::vector<std::vector<uint32_t>> detFaceIndices(numDets);
    std::vector<std::vector<uint32_t>> detWindingOrder(numDets);
    std::vector<int> detIds(numDets);

    extractFacesAndIDs(detFaces, windingOrder, vertices, detIdToIndex, faceIndices, detFaceVerts, detFaceIndices,
                       detWindingOrder, detIds);

    Pixels detectorPixels;
    bool calculatePixelCentre = true;
    if (detFaces.size() != 2 * numDets) {
      // At least one pixel is 3D (comprises multiple faces)
      // We need pixel offsets from the NXdetector in this case, as
      // calculating centre of mass for a general polyhedron is fairly
      // complex and computationally expensive
      Pixels pixelOffsets = getPixelOffsets(detectorGroup);
      // Calculate pixel relative positions
      detectorPixels = Eigen::Affine3d::Identity() * pixelOffsets;
      calculatePixelCentre = false;
    }

    for (size_t i = 0; i < numDets; ++i) {
      auto &detVerts = detFaceVerts[i];
      const auto &singleDetIndices = detFaceIndices[i];
      const auto &detWinding = detWindingOrder[i];

      Eigen::Vector3d centre;
      if (calculatePixelCentre) {
        // Our detector is 2D (described by a single face in the mesh)
        // Calculate polygon centre
        centre = std::accumulate(detVerts.begin() + 1, detVerts.end(), detVerts.front()) / detVerts.size();
      } else {
        // Our detector is 3D (described by multiple faces in the mesh)
        // Use pixel offset which was recorded in the NXdetector
        centre = detectorPixels.col(i);
      }

      // translate shape to origin for shape coordinates
      std::for_each(detVerts.begin(), detVerts.end(), [&centre](Eigen::Vector3d &val) { val -= centre; });

      auto shape = NexusShapeFactory::createFromOFFMesh(singleDetIndices, detWinding, detVerts);
      builder.addDetectorToLastBank(name + "_" + std::to_string(i), detIds[i], centre, std::move(shape));
    }
  }

  void parseMeshAndAddDetectors(InstrumentBuilder &builder, const Group &shapeGroup,
                                const std::vector<Mantid::detid_t> &detectorIds, const std::string &bankName,
                                const Group &detectorGroup) {
    // Load mapping between detector IDs and faces, winding order of vertices
    // for faces, and face corner vertices
    const auto detFaces = readNXUInts32(shapeGroup, "detector_faces");
    const auto faceIndices = readNXUInts32(shapeGroup, "faces");
    const auto windingOrder = readNXUInts32(shapeGroup, "winding_order");
    const auto vertices = readNXFloats(shapeGroup, "vertices");

    // Sanity check entries
    if (detFaces.size() < 2 * detectorIds.size())
      throw std::runtime_error("Expect to have at least as many detector_face "
                               "entries as detector_number entries");
    if (detFaces.size() % 2 != 0)
      throw std::runtime_error("Unequal pairs of face indices to detector "
                               "indices in detector_faces");
    if (detFaces.size() / 2 > faceIndices.size())
      throw std::runtime_error("Cannot have more detector_faces entries than faces entries");
    if (vertices.size() % 3 != 0)
      throw std::runtime_error("Unequal triple entries for vertices. Must be 3 * n entries");

    // Build a map of detector IDs to the index of occurrence in the
    // "detector_number" dataset
    std::unordered_map<int, uint32_t> detIdToIndex;
    for (uint32_t i = 0; i < detectorIds.size(); ++i) {
      detIdToIndex.emplace(detectorIds[i], i);
    }

    extractNexusMeshAndAddDetectors(detFaces, faceIndices, windingOrder, vertices, detectorIds.size(), detIdToIndex,
                                    bankName, builder, detectorGroup);
  }

  void parseAndAddBank(const Group &shapeGroup, InstrumentBuilder &builder,
                       const std::vector<Mantid::detid_t> &detectorIds, const std::string &bankName,
                       const Group &detectorGroup) {
    if (utilities::hasNXClass(shapeGroup, NX_OFF)) {
      parseMeshAndAddDetectors(builder, shapeGroup, detectorIds, bankName, detectorGroup);
    } else if (utilities::hasNXClass(shapeGroup, NX_CYLINDER)) {
      parseNexusCylinderDetector(shapeGroup, bankName, builder, detectorIds);
    } else {
      std::stringstream ss;
      ss << "Shape group " << H5_OBJ_NAME(shapeGroup) << " has unknown geometry type specified via " << NX_CLASS;
      throw std::runtime_error(ss.str());
    }
  }

  /**
   * Parse and return any sub-group providing shape information as Geometry
   * IObject.
   *
   * Null object return if no shape can be found.
   * @param detectorGroup : parent group possibly containing sub-group relating
   * to shape
   * @param searchTubes : out parameter, true if tubes can be searched
   * @return shared pointer holding IObject subtype or null shared pointer
   */
  std::shared_ptr<const Geometry::IObject> parseNexusShape(const Group &detectorGroup, bool &searchTubes) {
    // Note in the following we are NOT looking for named groups, only groups
    // that have NX_class attributes of either NX_CYLINDER or NX_OFF. That way
    // we handle groups called any of the allowed - shape, pixel_shape,
    // detector_shape
    auto cylindrical = utilities::findGroup(detectorGroup, NX_CYLINDER);
    auto off = utilities::findGroup(detectorGroup, NX_OFF);
    searchTubes = false;
    if (off && cylindrical) {
      throw std::runtime_error("Can either provide cylindrical OR OFF "
                               "geometries as subgroups, not both");
    }
    if (cylindrical) {
      searchTubes = true;
      return parseNexusCylinder(*cylindrical);
    } else if (off)
      return parseNexusMesh(*off);
    else {
      return std::shared_ptr<const Geometry::IObject>(nullptr);
    }
  }

  // Parse source and add to instrument
  void parseAndAddSource(const H5File &file, const Group &parent, InstrumentBuilder &builder) {
    Group instrumentGroup = utilities::findGroupOrThrow(parent, NX_INSTRUMENT);
    Group sourceGroup = utilities::findGroupOrThrow(instrumentGroup, NX_SOURCE);
    std::string sourceName = "Unspecified";
    if (utilities::findDataset(sourceGroup, "name"))
      sourceName = readOrSubstitute("name", sourceGroup, sourceName);
    auto sourceTransformations = getTransformations(file, sourceGroup);
    auto defaultPos = Eigen::Vector3d(0.0, 0.0, 0.0);
    builder.addSource(sourceName, sourceTransformations * defaultPos);
  }

  // Parse sample and add to instrument
  void parseAndAddSample(const H5File &file, const Group &parent, InstrumentBuilder &builder) {
    Group sampleGroup = utilities::findGroupOrThrow(parent, NX_SAMPLE);
    auto sampleTransforms = getTransformations(file, sampleGroup);
    Eigen::Vector3d samplePos = sampleTransforms * Eigen::Vector3d(0.0, 0.0, 0.0);
    std::string sampleName = "Unspecified";
    if (utilities::findDataset(sampleGroup, "name"))
      sampleName = readOrSubstitute("name", sampleGroup, sampleName);
    builder.addSample(sampleName, samplePos);
  }

  void parseMonitors(const H5File &file, const H5::Group &parent, InstrumentBuilder &builder) {
    // As for `openDetectorGroups`: this method was previously written to parse the monitors from every instrument in
    // the file. But then was only used with files containing a single instrument.

    // In order to be used with files containing multiple workspaces, the requirement is actually
    // to parse _only_ the monitors from the current instrument.

    Group instrumentGroup = utilities::findGroupOrThrow(parent, NX_INSTRUMENT);

    std::vector<Group> monitorGroups = openSubGroups(instrumentGroup, NX_MONITOR);
    for (const auto &monitor : monitorGroups) {
      if (!utilities::findDataset(monitor, DETECTOR_ID))
        throw std::invalid_argument("NXmonitors must have " + DETECTOR_ID);
      auto detectorId = readNXInts(monitor, DETECTOR_ID)[0];
      bool proxy = false;
      auto monitorShape = parseNexusShape(monitor, proxy);
      auto monitorTransforms = getTransformations(file, monitor);
      builder.addMonitor(std::to_string(detectorId), static_cast<Mantid::detid_t>(detectorId),
                         monitorTransforms * Eigen::Vector3d{0, 0, 0}, monitorShape);
    }
  }

public:
  explicit Parser(std::unique_ptr<AbstractLogger> &&logger) : m_logger(std::move(logger)) {}

  std::unique_ptr<const Mantid::Geometry::Instrument> extractInstrument(const H5File &file, const Group &parent) {
    InstrumentBuilder builder(instrumentName(parent));

    // Open all detector subgroups
    const std::vector<Group> detectorGroups = openDetectorGroups(parent);
    for (auto &detectorGroup : detectorGroups) {
      // Transform in homogenous coordinates. Offsets will be rotated then bank
      // translation applied.
      auto debug = H5_OBJ_NAME(detectorGroup);
      Eigen::Transform<double, 3, 2> transforms = getTransformations(file, detectorGroup);
      // Absolute bank position
      Eigen::Vector3d bankPos = transforms * Eigen::Vector3d{0, 0, 0};
      // Absolute bank rotation
      auto bankRotation = Eigen::Quaterniond(transforms.rotation());
      std::string bankName;
      if (utilities::findDataset(detectorGroup, BANK_NAME))
        bankName = get1DStringDataset(BANK_NAME,
                                      detectorGroup); // local_name is optional
      builder.addBank(bankName, bankPos, bankRotation);
      // Get the pixel detIds
      auto detectorIds = getDetectorIds(detectorGroup);

      // We preferentially deal with DETECTOR_SHAPE type shapes. Pixel offsets
      // only needed if pixels are 3D for this processing
      auto detector_shape = utilities::findGroupByName(detectorGroup, DETECTOR_SHAPE);
      if (detector_shape) {
        parseAndAddBank(*detector_shape, builder, detectorIds, bankName, detectorGroup);
        continue;
      }

      // Get the pixel offsets
      Pixels pixelOffsets = getPixelOffsets(detectorGroup);
      // Calculate pixel relative positions
      Pixels detectorPixels = Eigen::Affine3d::Identity() * pixelOffsets;
      bool searchTubes = false;
      // Extract shape
      auto detShape = parseNexusShape(detectorGroup, searchTubes);

      if (searchTubes) {
        auto tubes = TubeHelpers::findAndSortTubes(*detShape, detectorPixels, detectorIds);
        builder.addTubes(bankName, tubes, detShape);

        // Even if tubes are searched, we do NOT guarantee all detectors will be
        // in tube formation, so must continue to process non-tube detectors
        detectorIds = TubeHelpers::notInTubes(tubes, detectorIds);
      }
      for (size_t i = 0; i < detectorIds.size(); ++i) {
        auto index = static_cast<int>(i);
        std::string name = bankName + "_" + std::to_string(index);

        const Eigen::Vector3d &relativePos = detectorPixels.col(index);
        builder.addDetectorToLastBank(name, detectorIds[index], relativePos, detShape);
      }
    }

    // TODO? Sort the detectors

    // Parse the source and sample and add to instrument
    parseAndAddSample(file, parent, builder);
    parseAndAddSource(file, parent, builder);

    // Parse and add the monitors
    parseMonitors(file, parent, builder);

    return builder.createInstrument();
  }
};
} // namespace

std::unique_ptr<const Mantid::Geometry::Instrument>
NexusGeometryParser::createInstrument(const std::string &fileName, std::unique_ptr<AbstractLogger> logger) {
  const H5File file(fileName, H5F_ACC_RDONLY);
  auto rootGroup = file.openGroup("/");
  auto parentGroup = utilities::findGroupOrThrow(rootGroup, NX_ENTRY);

  Parser parser(std::move(logger));
  return parser.extractInstrument(file, parentGroup);
}

std::unique_ptr<const Geometry::Instrument>
NexusGeometryParser::createInstrument(const std::string &fileName, const std::string &parentGroupName,
                                      std::unique_ptr<AbstractLogger> logger) {

  const H5File file(fileName, H5F_ACC_RDONLY);
  auto parentGroup = file.openGroup(std::string("/") + parentGroupName);

  Parser parser(std::move(logger));
  return parser.extractInstrument(file, parentGroup);
}

// Create a unique instrument name from Nexus file
std::string NexusGeometryParser::getMangledName(const std::string &fileName, const std::string &instName) {
  std::string mangledName = instName;
  if (!fileName.empty()) {
    std::string checksum = Mantid::Kernel::ChecksumHelper::sha1FromString(fileName);
    mangledName += checksum;
  }
  return mangledName;
}
} // namespace Mantid::NexusGeometry
