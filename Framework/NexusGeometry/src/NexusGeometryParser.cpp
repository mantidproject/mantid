#include <iostream>
//
// Created by michael on 23/08/17.
//

//----------------------
// Includes
//----------------------

#include "MantidNexusGeometry/NexusGeometryParser.h"

#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace NexusGeometry {

using namespace H5;

// Eigen typedefs
typedef Eigen::Matrix<double, 3, Eigen::Dynamic> Pixels;

// Anonymous namespace
namespace {
const H5G_obj_t GROUP_TYPE = static_cast<H5G_obj_t>(0);
const H5std_string NX_CLASS = "NX_class";
const H5std_string NX_ENTRY = "NXentry";
const H5std_string NX_INSTRUMENT = "NXinstrument";
const H5std_string NX_DETECTOR = "NXdetector";
const H5std_string NX_MONITOR = "NXmonitor";
const H5std_string DETECTOR_IDS = "detector_number";
const H5std_string DETECTOR_ID = "detector_id";
const H5std_string X_PIXEL_OFFSET = "x_pixel_offset";
const H5std_string Y_PIXEL_OFFSET = "y_pixel_offset";
const H5std_string Z_PIXEL_OFFSET = "z_pixel_offset";
const H5std_string DEPENDS_ON = "depends_on";
const H5std_string NO_DEPENDENCY = ".";
const H5std_string PIXEL_SHAPE = "pixel_shape";
const H5std_string SHAPE = "shape";
// Transformation types
const H5std_string TRANSFORMATION_TYPE = "transformation_type";
const H5std_string TRANSLATION = "translation";
const H5std_string ROTATION = "rotation";
const H5std_string VECTOR = "vector";
const H5std_string UNITS = "units";
// Radians and degrees
const H5std_string DEGREES = "degrees";
const static double PI = 3.1415926535;
const static double DEGREES_IN_SEMICIRCLE = 180;
// Nexus shape types
const H5std_string NX_CYLINDER = "NXcylindrical_geometry";
const H5std_string NX_OFF = "NXoff_geometry";

std::vector<uint16_t> vecUnsignedInt16(const std::vector<int32_t> &toConvert) {
  std::vector<uint16_t> target(toConvert.size());
  for (size_t i = 0; i < target.size(); ++i) {
    target[i] = uint16_t(toConvert[i]);
  }
  return target;
}

} // namespace

/// Constructor opens the nexus file
NexusGeometryParser::NexusGeometryParser(
    const H5std_string &fileName, iAbstractBuilder_sptr iAbsBuilder_sptr) {

  // Disable automatic printing, so Load algorithm can deal with errors
  // appropriately
  Exception::dontPrint();
  try {
    H5File file(fileName, H5F_ACC_RDONLY);
    this->nexusFile = file;
    this->rootGroup = this->nexusFile.openGroup("/");
  } catch (FileIException &e) {
    this->exitStatus = OPENING_FILE_ERROR;
  } catch (GroupIException &e) {
    this->exitStatus = OPENING_ROOT_GROUP_ERROR;
  }
  // Initialize the instrumentAbstractBuilder
  this->iBuilder_sptr = std::move(iAbsBuilder_sptr);
}

/// OFF NEXUS GEOMETRY PARSER
ParsingErrors NexusGeometryParser::parseNexusGeometry() {
  // Determine if nexusFile was successfully opened
  switch (this->exitStatus) {
  case NO_ERROR:
    break;
  default:
    return this->exitStatus;
  }

  // Get path to all detector groups
  try {
    std::vector<Group> detectorGroups = this->openDetectorGroups();
    for (auto &detectorGroup : detectorGroups) {
      // Get the pixel offsets
      Pixels pixelOffsets = this->getPixelOffsets(detectorGroup);
      // Get the transformations
      Eigen::Transform<double, 3, 2> transforms =
          this->getTransformations(detectorGroup);
      // Calculate pixel positions
      Pixels detectorPixels = transforms * pixelOffsets;
      // Get the pixel detIds
      std::vector<int> detectorIds = this->getDetectorIds(detectorGroup);
      // Extract shape
      auto shape = this->parseNexusShape(detectorGroup);

      for (size_t i = 0; i < detectorIds.size(); ++i) {
        auto index = static_cast<int>(i);
        std::string name = std::to_string(index);
        Eigen::Vector3d detPos = detectorPixels.col(index);
        this->iBuilder_sptr->addDetector(name, detectorIds[index], detPos,
                                         shape);
      }
    }
    // Sort the detectors
    this->iBuilder_sptr->sortDetectors();
    // Parse source and sample and add to instrument
    this->parseAndAddSource();
    this->parseAndAddSample();
    this->parseMonitors();
  } catch (H5::Exception &ex) {
    this->exitStatus = UNKNOWN_ERROR;
  }

  return this->exitStatus;
}

/// Open subgroups of parent group
std::vector<Group>
NexusGeometryParser::openSubGroups(Group &parentGroup,
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

// Open all detector groups into a vector
std::vector<Group> NexusGeometryParser::openDetectorGroups() {
  std::vector<Group> rawDataGroupPaths =
      this->openSubGroups(this->rootGroup, NX_ENTRY);

  // Open all instrument groups within rawDataGroups
  std::vector<Group> instrumentGroupPaths;
  for (auto rawDataGroupPath : rawDataGroupPaths) {
    std::vector<Group> instrumentGroups =
        this->openSubGroups(rawDataGroupPath, NX_INSTRUMENT);
    instrumentGroupPaths.insert(instrumentGroupPaths.end(),
                                instrumentGroups.begin(),
                                instrumentGroups.end());
  }
  // Open all detector groups within instrumentGroups
  std::vector<Group> detectorGroupPaths;
  for (auto instrumentGroupPath : instrumentGroupPaths) {
    // Open sub detector groups
    std::vector<Group> detectorGroups =
        this->openSubGroups(instrumentGroupPath, NX_DETECTOR);
    // Append to detectorGroups vector
    detectorGroupPaths.insert(detectorGroupPaths.end(), detectorGroups.begin(),
                              detectorGroups.end());
  }
  // Return the detector groups
  return detectorGroupPaths;
}

// Function to return the detector ids in the same order as the offsets
std::vector<int> NexusGeometryParser::getDetectorIds(Group &detectorGroup) {

  std::vector<int> detIds;

  for (unsigned int i = 0; i < detectorGroup.getNumObjs(); ++i) {
    H5std_string objName = detectorGroup.getObjnameByIdx(i);
    if (objName == DETECTOR_IDS) {
      detIds = this->get1DDataset<int>(objName, detectorGroup);
    }
  }
  return detIds;
}

// Function to return the (x,y,z) offsets of pixels in the chosen detectorGroup
Pixels NexusGeometryParser::getPixelOffsets(Group &detectorGroup) {

  // Initialise matrix
  Pixels offsetData;
  std::vector<double> xValues, yValues, zValues;
  for (unsigned int i = 0; i < detectorGroup.getNumObjs(); i++) {
    H5std_string objName = detectorGroup.getObjnameByIdx(i);
    if (objName == X_PIXEL_OFFSET) {
      xValues = this->get1DDataset<double>(objName, detectorGroup);
    }
    if (objName == Y_PIXEL_OFFSET) {
      yValues = this->get1DDataset<double>(objName, detectorGroup);
    }
    if (objName == Z_PIXEL_OFFSET) {
      zValues = this->get1DDataset<double>(objName, detectorGroup);
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

// Function to read in a dataset into a vector
template <typename valueType>
std::vector<valueType>
NexusGeometryParser::get1DDataset(const H5std_string &dataset) {
  // Open data set
  DataSet data = this->nexusFile.openDataSet(dataset);
  DataSpace dataSpace = data.getSpace();
  std::vector<valueType> values;
  values.resize(dataSpace.getSelectNpoints());
  // Read data into vector
  data.read(values.data(), data.getDataType(), dataSpace);

  // Return the data vector
  return values;
}

// Function to read in a dataset into a vector
template <typename valueType>
std::vector<valueType>
NexusGeometryParser::get1DDataset(const H5std_string &dataset,
                                  const H5::Group &group) {
  // Open data set
  DataSet data = group.openDataSet(dataset);
  DataSpace dataSpace = data.getSpace();
  std::vector<valueType> values;
  values.resize(dataSpace.getSelectNpoints());
  // Read data into vector
  data.read(values.data(), data.getDataType(), dataSpace);

  // Return the data vector
  return values;
}

std::string NexusGeometryParser::get1DStringDataset(const std::string &dataset,
                                                    const Group &group) {
  // Open data set
  DataSet data = group.openDataSet(dataset);
  auto dataType = data.getDataType();
  auto nCharacters = dataType.getSize();
  std::vector<char> value(nCharacters);
  data.read(value.data(), dataType, data.getSpace());
  return std::string(value.begin(), value.end());
}

// Function to get the transformations from the nexus file, and create the Eigen
// transform object
Eigen::Transform<double, 3, Eigen::Affine>
NexusGeometryParser::getTransformations(const Group &detectorGroup) {
  H5std_string dependency;
  // Get absolute dependency path
  try {
    dependency = this->get1DStringDataset(DEPENDS_ON, detectorGroup);
  } catch (H5::GroupIException &) {
    return Eigen::Transform<double, 3, Eigen::Affine>::Identity();
  }

  // Initialise transformation holder as zero-degree rotation
  Eigen::Transform<double, 3, Eigen::Affine> transforms;
  Eigen::Vector3d axis(1.0, 0.0, 0.0);
  transforms = Eigen::AngleAxisd(0.0, axis);

  // Breaks when no more dependencies (dependency = ".")
  // Transformations must be applied in the order of direction of discovery
  // (they are _passive_ transformations)
  while (dependency != NO_DEPENDENCY) {
    // Open the transformation data set
    DataSet transformation = this->nexusFile.openDataSet(dependency);

    // Get magnitude of current transformation
    double magnitude = this->get1DDataset<double>(dependency)[0];
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
      if (transformUnits == DEGREES) {
        // Convert angle from degrees to radians
        angle *= PI / DEGREES_IN_SEMICIRCLE;
      }
      Eigen::AngleAxisd rotation(angle, transformVector);
      transforms = rotation * transforms;
    }
  }
  return transforms;
}

/// Choose what shape type to parse
objectHolder NexusGeometryParser::parseNexusShape(const Group &detectorGroup) {
  Group shapeGroup;
  try {
    shapeGroup = detectorGroup.openGroup(PIXEL_SHAPE);
  } catch (...) {
    // TODO. Current assumption. Can we have pixels without specifying a shape?
    try {
      shapeGroup = detectorGroup.openGroup(SHAPE);
    } catch (...) {
      return objectHolder(nullptr);
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
    return this->parseNexusCylinder(shapeGroup);
  } else if (shapeType == NX_OFF) {
    return this->parseNexusMesh(shapeGroup);
  } else {
    throw std::runtime_error(
        "Shape type not recognised by NexusGeometryParser");
  }
}

// Parse cylinder nexus geometry
objectHolder NexusGeometryParser::parseNexusCylinder(const Group &shapeGroup) {
  H5std_string pointsToVertices = "cylinders";
  std::vector<int> cPoints =
      this->get1DDataset<int>(pointsToVertices, shapeGroup);

  H5std_string verticesData = "vertices";
  // 1D reads row first, then columns
  std::vector<double> vPoints =
      this->get1DDataset<double>(verticesData, shapeGroup);
  Eigen::Map<Eigen::Matrix<double, 3, 3>> vertices(vPoints.data());
  // Read points into matrix, sorted by cPoints ordering
  Eigen::Matrix<double, 3, 3> vSorted;
  for (int i = 0; i < 3; ++i) {
    vSorted.col(cPoints[i]) = vertices.col(i);
  }
  return this->sAbsCreator.createCylinder(vSorted);
}

// Parse OFF (mesh) nexus geometry
objectHolder NexusGeometryParser::parseNexusMesh(const Group &shapeGroup) {

  const std::vector<uint16_t> faceIndices =
      vecUnsignedInt16(this->get1DDataset<int32_t>("faces", shapeGroup));
  const std::vector<uint16_t> windingOrder = vecUnsignedInt16(
      this->get1DDataset<int32_t>("winding_order", shapeGroup));
  std::vector<uint16_t> triangularFaces =
      createTriangularFaces(faceIndices, windingOrder);

  // 1D reads row first, then columns
  const auto nexusVertices = this->get1DDataset<double>("vertices", shapeGroup);
  auto numberOfVertices = nexusVertices.size() / 3;
  std::vector<Mantid::Kernel::V3D> vertices(numberOfVertices);
  for (size_t vertexNumber = 0; vertexNumber < nexusVertices.size();
       vertexNumber += 3) {
    vertices[vertexNumber / 3] = Mantid::Kernel::V3D(
        nexusVertices[vertexNumber], nexusVertices[vertexNumber + 1],
        nexusVertices[vertexNumber + 2]);
  }

  return this->sAbsCreator.createMesh(std::move(triangularFaces),
                                      std::move(vertices));
}

std::vector<uint16_t> NexusGeometryParser::createTriangularFaces(
    const std::vector<uint16_t> &faceIndices,
    const std::vector<uint16_t> &windingOrder) const {

  // Elements 0 to 2 are the indices of the vertices vector corresponding to the
  // vertices of the first triangle.
  // Elements 3 to 5 are for the second triangle, and so on.
  // The order of the vertices is the winding order of the triangle, determining
  // the face normal by right-hand rule
  std::vector<uint16_t> triangularFaces;

  int startOfFace = 0;
  int endOfFace = 0;
  for (auto it = faceIndices.begin() + 1; it != faceIndices.end(); ++it) {
    endOfFace = *it;
    createTrianglesFromPolygon(windingOrder, triangularFaces, startOfFace,
                               endOfFace);
  }

  // and the last face
  endOfFace = static_cast<int>(windingOrder.size());
  createTrianglesFromPolygon(windingOrder, triangularFaces, startOfFace,
                             endOfFace);

  return triangularFaces;
}

void NexusGeometryParser::createTrianglesFromPolygon(
    const std::vector<uint16_t> &windingOrder,
    std::vector<uint16_t> &triangularFaces, int &startOfFace,
    int &endOfFace) const {
  int polygonOrder = endOfFace - startOfFace;
  auto first = windingOrder.begin() + startOfFace;

  for (int polygonVertex = 1; polygonVertex < polygonOrder - 1;
       ++polygonVertex) {
    triangularFaces.push_back(*first);
    triangularFaces.push_back(*(first + polygonVertex));
    triangularFaces.push_back(*(first + polygonVertex + 1));
  }
  startOfFace = endOfFace; // start of the next face
}

// Parse source and add to instrument
void NexusGeometryParser::parseAndAddSource() {
  H5std_string sourcePath = "raw_data_1/instrument/source";
  Group sourceGroup = this->rootGroup.openGroup(sourcePath);
  auto sourceName = this->get1DStringDataset("name", sourceGroup);
  auto sourceTransformations = this->getTransformations(sourceGroup);
  auto defaultPos = Eigen::Vector3d(0.0, 0.0, 0.0);
  this->iBuilder_sptr->addSource(sourceName,
                                 sourceTransformations * defaultPos);
}
// Parse sample and add to instrument
void NexusGeometryParser::parseAndAddSample() {
  std::string sampleName = "sample";
  H5std_string samplePath = "raw_data_1/sample";
  Group sampleGroup = this->rootGroup.openGroup(samplePath);
  auto sampleTransforms = this->getTransformations(sampleGroup);
  auto samplePos = sampleTransforms * Eigen::Vector3d(0.0, 0.0, 0.0);

  this->iBuilder_sptr->addSample(sampleName, samplePos);
}

void NexusGeometryParser::parseMonitors() {
  std::vector<Group> rawDataGroupPaths =
      this->openSubGroups(this->rootGroup, NX_ENTRY);

  // Open all instrument groups within rawDataGroups
  for (auto rawDataGroupPath : rawDataGroupPaths) {
    std::vector<Group> instrumentGroups =
        this->openSubGroups(rawDataGroupPath, NX_INSTRUMENT);
    for (auto &inst : instrumentGroups) {
      std::vector<Group> monitorGroups = this->openSubGroups(inst, NX_MONITOR);
      for (auto &monitor : monitorGroups) {

        int detectorId = get1DDataset<int>(DETECTOR_ID, monitor)[0];

        objectHolder monitorShape = parseNexusShape(monitor);
        this->iBuilder_sptr->addMonitor(std::to_string(detectorId), detectorId,
                                        Eigen::Vector3d{0, 0, 0}, monitorShape);
      }
    }
  }
}
} // namespace NexusGeometry
} // namespace Mantid
