// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/Mantid3MFFileIO.h"
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MaterialXMLParser.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/StringTokenizer.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeList.h"
#include <Poco/DOM/Element.h>
#include <boost/algorithm/string/trim.hpp>
#include <iostream>

namespace {
constexpr auto SAMPLE_OBJECT_NAME = "SAMPLE";
constexpr double M_TOLERANCE = 0.000001;
} // namespace

namespace Mantid {
namespace DataHandling {

/**
 * Load 3MF format file
 * @param filename The name of the 3MF file
 */
void Mantid3MFFileIO::LoadFile(std::string filename) {

  // Import Model from 3MF File
  {
    Lib3MF::PReader reader = model->QueryReader("3mf");
    // And deactivate the strict mode (default is "false", anyway. This just
    // demonstrates where/how to use it).
    reader->SetStrictModeActive(false);
    reader->ReadFromFile(filename);
    m_filename = filename;

    ScaleUnits scale;
    switch (model->GetUnit()) {
    case Lib3MF::eModelUnit::MilliMeter:
      scale = ScaleUnits::millimetres;
      break;
    case Lib3MF::eModelUnit::CentiMeter:
      scale = ScaleUnits::centimetres;
      break;
    case Lib3MF::eModelUnit::Meter:
      scale = ScaleUnits::metres;
      break;
    default:
      // reader defaults to mm for unknown units. For non-metric do likewise
      // here
      g_log.warning("Only m, cm and mm are supported in Mantid");
      scale = ScaleUnits::millimetres;
    };
    setScaleType(scale);

    for (Lib3MF_uint32 iWarning = 0; iWarning < reader->GetWarningCount(); iWarning++) {
      Lib3MF_uint32 nErrorCode;
      std::string sWarningMessage = reader->GetWarning(iWarning, nErrorCode);
      std::stringstream ss;
      ss << "Encountered warning #" << nErrorCode << " : " << sWarningMessage << std::endl;
      g_log.warning(ss.str());
    }
  }
}

/**
 * Load a single mesh object into a Mantid Geometry::MeshObject
 * @param meshObject A Lib3mf pointer to the mesh object
 * @param buildTransform The rotation/translation to be applied to the object
 * @return A pointer to the Geometry::MeshObject
 */
MeshObject_sptr Mantid3MFFileIO::loadMeshObject(Lib3MF::PMeshObject meshObject, sLib3MFTransform buildTransform) {

  Lib3MF_uint64 nVertexCount = meshObject->GetVertexCount();
  Lib3MF_uint64 nTriangleCount = meshObject->GetTriangleCount();

  g_log.debug("Mesh loaded");

  g_log.debug("Name: \"" + meshObject->GetName() + "\"");
  g_log.debug("PartNumber: \"" + meshObject->GetPartNumber() + "\"");
  g_log.debug("Vertex count: " + std::to_string(nVertexCount));
  g_log.debug("Triangle count: " + std::to_string(nTriangleCount));

  uint32_t vertexCount = 0;
  std::vector<Lib3MF::sTriangle> triangles;
  meshObject->GetTriangleIndices(triangles);

  m_triangle.clear();
  m_vertices.clear();

  for (auto i : triangles) {
    m_triangle.push_back(i.m_Indices[0]);
    m_triangle.push_back(i.m_Indices[1]);
    m_triangle.push_back(i.m_Indices[2]);
  }
  std::vector<Lib3MF::sPosition> vertices;
  meshObject->GetVertices(vertices);
  for (auto i : vertices) {
    Mantid::Kernel::V3D vertex = createScaledV3D(i.m_Coordinates[0], i.m_Coordinates[1], i.m_Coordinates[2]);
    m_vertices.push_back(vertex);
  }

  Mantid::Kernel::Material material;

  Lib3MF_uint32 nResourceID;
  Lib3MF_uint32 nPropertyID;

  // load the material from the pid\pindex attributes on the mesh object
  if (meshObject->GetObjectLevelProperty(nResourceID, nPropertyID)) {

    Lib3MF::ePropertyType propType = model->GetPropertyTypeByID(nResourceID);

    if (propType == Lib3MF::ePropertyType::BaseMaterial) {

      Lib3MF::PBaseMaterialGroup baseMaterialGroup = model->GetBaseMaterialGroupByID(nResourceID);
      ReadMaterial::MaterialParameters params;
      std::string fullMaterialName = baseMaterialGroup->GetName(nPropertyID);
      std::string materialName;
      size_t openBracket = fullMaterialName.find("(");
      size_t closeBracket = fullMaterialName.find(")");

      materialName = fullMaterialName.substr(0, openBracket);
      boost::algorithm::trim(materialName);
      std::string xmlString;

      xmlString = "<material id=\"" + materialName + "\" formula=\"" + materialName + "\"";
      if ((openBracket != std::string::npos) && (closeBracket != std::string::npos)) {
        std::string materialSpec = fullMaterialName.substr(openBracket + 1, closeBracket - openBracket - 1);
        xmlString += " " + materialSpec;
      }
      xmlString += "></material>";
      Poco::XML::DOMParser parser;
      Poco::XML::AutoPtr<Poco::XML::Document> doc;
      try {
        doc = parser.parseString(xmlString);
        Poco::XML::AutoPtr<Poco::XML::NodeList> materialElements = doc->getElementsByTagName("material");
        Kernel::MaterialXMLParser materialParser;
        material = materialParser.parse(static_cast<Poco::XML::Element *>(materialElements->item(0)), m_filename);
      } catch (std::exception &e) {
        g_log.warning("Unable to parse material properties for " + fullMaterialName +
                      " so material will be ignored: " + e.what());
      };
    }
  };

  auto mesh = std::make_shared<Geometry::MeshObject>(std::move(m_triangle), std::move(m_vertices), material);
  mesh->setID(meshObject->GetName());

  // 3MF stores transformation as a 4 x 3 matrix using row major convention
  // The 4th column is implicit (0,0,0,1)
  Kernel::Matrix<double> transformMatrix(4, 4);

  // copy data into Mantid matrix and transpose
  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 3; j++) {
      transformMatrix[j][i] = buildTransform.m_Fields[i][j];
    }
  }
  // fill in the implicit row and make explicit
  transformMatrix.setRow(3, {0, 0, 0, 1});
  // scale the translations into metres
  for (size_t i = 0; i < 3; i++) {
    transformMatrix[i][3] = scaleValue(transformMatrix[i][3]);
  }

  mesh->multiply(transformMatrix);

  return mesh;
}

/**
 * Read a set of mesh objects from the in memory lib3mf model object
 * @param meshObjects A vector to store the meshes for env components
 * @param sample A parameter to store a mesh for the sample
 */
void Mantid3MFFileIO::readMeshObjects(std::vector<MeshObject_sptr> &meshObjects, MeshObject_sptr &sample) {
  Lib3MF::PBuildItemIterator buildItemIterator = model->GetBuildItems();
  while (buildItemIterator->MoveNext()) {
    Lib3MF::PBuildItem buildItem = buildItemIterator->GetCurrent();
    uint32_t objectResourceID = buildItem->GetObjectResourceID();
    sLib3MFTransform transform = buildItem->GetObjectTransform();
    readMeshObject(meshObjects, sample, objectResourceID, transform);
  }
}

/**
 * Attempt to read a single mesh from a specified lib3mf resource id. If the
 * resource id points at a component instead then read them
 * @param meshObjects A vector to store the meshes for env components
 * @param sample A parameter to store a mesh for the sample
 * @param objectResourceID Integer identifier of the object to read in
 * @param transform Rotation/translation matrix for the object
 */
void Mantid3MFFileIO::readMeshObject(std::vector<MeshObject_sptr> &meshObjects, MeshObject_sptr &sample,
                                     uint32_t objectResourceID, sLib3MFTransform transform) {
  // no general GetObjectByID in the lib3MF library??
  try {
    Lib3MF::PMeshObject meshObject = model->GetMeshObjectByID(objectResourceID);
    std::string objectName = meshObject->GetName();
    std::transform(objectName.begin(), objectName.end(), objectName.begin(), toupper);
    if (objectName == SAMPLE_OBJECT_NAME) {
      sample = loadMeshObject(meshObject, transform);
    } else {
      meshObjects.push_back(loadMeshObject(meshObject, transform));
    }
  } catch (std::exception) {
    readComponents(meshObjects, sample, objectResourceID, transform);
  }
}
/*
 * Read in the set of mesh objects pointed to by a component object
 * @param meshObjects A vector to store the meshes for env components
 * @param sample A parameter to store a mesh for the sample
 * @param objectResourceID Integer identifier of the object to read in
 * @param transform Rotation/translation matrix for the object
 */
void Mantid3MFFileIO::readComponents(std::vector<MeshObject_sptr> &meshObjects, MeshObject_sptr &sample,
                                     uint32_t objectResourceID, sLib3MFTransform transform) {
  Lib3MF::PComponentsObject componentsObject = model->GetComponentsObjectByID(objectResourceID);
  for (Lib3MF_uint32 nIndex = 0; nIndex < componentsObject->GetComponentCount(); nIndex++) {
    Lib3MF::PComponent component = componentsObject->GetComponent(nIndex);
    readMeshObject(meshObjects, sample, component->GetObjectResourceID(), transform);
  }
}

/*
 * Write a Mantid Geometry::MeshObjects to the lib3mf model object as part
 * of processing of writing to a 3mf format file
 * @param mantidMeshObject MeshObject to write out
 * @param name Name of the mesh object
 */
void Mantid3MFFileIO::writeMeshObject(const Geometry::MeshObject &mantidMeshObject, std::string name) {
  Lib3MF::PMeshObject meshObject = model->AddMeshObject();
  meshObject->SetName(name);
  std::vector<uint32_t> mantidTriangles = mantidMeshObject.getTriangles();
  std::vector<Kernel::V3D> mantidVertices = mantidMeshObject.getV3Ds();
  std::vector<sLib3MFTriangle> triangles;
  std::vector<sLib3MFPosition> vertices;

  // convert vertices from V3D to the Lib3MF struct
  for (auto i : mantidVertices) {
    sLib3MFPosition vertex = {
        {static_cast<Lib3MF_single>(i.X()), static_cast<Lib3MF_single>(i.Y()), static_cast<Lib3MF_single>(i.Z())}};
    vertices.push_back(vertex);
  }

  auto numTriangles = mantidMeshObject.numberOfTriangles();
  for (size_t i = 0; i < numTriangles; i++) {
    sLib3MFTriangle triangle;

    Kernel::V3D a = mantidVertices[mantidTriangles[3 * i]];
    Kernel::V3D b = mantidVertices[mantidTriangles[3 * i + 1]];
    Kernel::V3D c = mantidVertices[mantidTriangles[3 * i + 2]];
    Kernel::V3D centroid = (a + b + c) / 3;
    Kernel::V3D b_minus_a = b - a;
    Kernel::V3D c_minus_a = c - a;
    Kernel::V3D faceNormal = normalize(b_minus_a.cross_prod(c_minus_a));
    Geometry::Track faceNormalTrack = Geometry::Track(centroid, faceNormal);
    mantidMeshObject.interceptSurface(faceNormalTrack);

    // check if first link has any distance inside object
    auto firstLink = faceNormalTrack.front();

    if (firstLink.distInsideObject > M_TOLERANCE) {
      g_log.debug("Face normal pointing to interior of object on object " + mantidMeshObject.id() +
                  ". Vertices swapped");
      // swap order of b and c
      mantidVertices[3 * i + 1] = c;
      mantidVertices[3 * i + 2] = b;
    }

    for (int j = 0; j < 3; j++) {
      triangle.m_Indices[j] = mantidTriangles[3 * i + j];
    }

    triangles.push_back(triangle);
  }
  meshObject->SetGeometry(vertices, triangles);

  if (!mantidMeshObject.material().name().empty()) {
    Lib3MF_uint32 materialPropertyID;
    int baseMaterialsResourceID;

    AddBaseMaterial(mantidMeshObject.material().name(), generateRandomColor(), baseMaterialsResourceID,
                    materialPropertyID);
    meshObject->SetObjectLevelProperty(baseMaterialsResourceID, materialPropertyID);
  }

  // Set up one to one mapping between build items and mesh objects
  // Don't bother setting up any components
  sLib3MFTransform mMatrix = {{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}}};
  model->AddBuildItem(meshObject.get(), mMatrix);
}

/*
 * Generate a random colour - to be used for a mesh object
 * being written out to a .3mf format file
 * @return RGB colour value
 */
int Mantid3MFFileIO ::generateRandomColor() {

  int rColor = (rand() % 256) << 16;
  int gColor = (rand() % 256) << 8;
  int bColor = (rand() % 256) << 0;

  return rColor | gColor | bColor;
}

/* Basic write to 3MF. Since Mantid is storing each instance of a shape
 * as a separate mesh there's no possiblity of reusing meshes as supported
 * by 3MF
 * @param meshObjects vector of Mantid mesh objects for environment components
 * to write out to file
 * @param sample Mesh Object representing sample
 * @param scaleType Units to write out coordinates in eg mm
 */
void Mantid3MFFileIO::writeMeshObjects(std::vector<const Geometry::MeshObject *> meshObjects,
                                       MeshObject_const_sptr &sample, DataHandling::ScaleUnits scaleType) {

  Lib3MF::eModelUnit scale;
  switch (scaleType) {
  case ScaleUnits::millimetres:
    scale = Lib3MF::eModelUnit::MilliMeter;
    break;
  case ScaleUnits::centimetres:
    scale = Lib3MF::eModelUnit::CentiMeter;
    break;
  case ScaleUnits::metres:
    scale = Lib3MF::eModelUnit::Meter;
    break;
  default:
    throw "Units not supported";
  };
  model->SetUnit(scale);

  writeMeshObject(*sample, SAMPLE_OBJECT_NAME);

  for (auto mantidMeshObject : meshObjects) {
    writeMeshObject(*mantidMeshObject, mantidMeshObject->id());
  }
}

/*
 * Add a new material to the lib3mf model object in preparation for writing
 * a 3mf format file
 * @param materialName Name of material to be added
 * @param materialColor RGB colour to be used to display material in CAD
 * applications
 * @param resourceID Out parameter representing object id of material created
 * @param materialPropertyID Out parameter representing id of material created
 */
void Mantid3MFFileIO::AddBaseMaterial(std::string materialName, int materialColor, int &resourceID,
                                      Lib3MF_uint32 &materialPropertyID) {
  // check if master material definition exists
  bool materialNameExists = false;

  Lib3MF::PBaseMaterialGroupIterator materialIterator = model->GetBaseMaterialGroups();
  Lib3MF::PBaseMaterialGroup groupToAddTo;

  if (materialIterator->Count() == 0) {
    groupToAddTo = model->AddBaseMaterialGroup();
  } else {
    while (materialIterator->MoveNext() && !materialNameExists) {
      Lib3MF::PBaseMaterialGroup materialGroup = materialIterator->GetCurrentBaseMaterialGroup();
      // by default add the new material to the 1st material group unless
      // another group is found with the supplied materialName
      if (!groupToAddTo) {
        groupToAddTo = materialGroup;
      }
      std::vector<Lib3MF_uint32> materialPropertyIDs;
      materialGroup->GetAllPropertyIDs(materialPropertyIDs);
      for (auto i : materialPropertyIDs) {
        std::string existingMaterialName = materialGroup->GetName(i);
        if (materialName == existingMaterialName) {
          materialNameExists = true;
          groupToAddTo = materialGroup;
          materialPropertyID = i;
          break;
        }
      }
    }
  }
  Lib3MF_uint8 rColor = (materialColor & 0xFF0000) >> 16;
  Lib3MF_uint8 gColor = (materialColor & 0x00FF00) >> 8;
  Lib3MF_uint8 bColor = (materialColor & 0x0000FF) >> 0;
  if (!materialNameExists) {
    materialPropertyID = groupToAddTo->AddMaterial(materialName, Lib3MF::sColor{rColor, gColor, bColor, 255});
  }
  resourceID = groupToAddTo->GetResourceID();
}

/*
 * Assign a material to a mesh object. If master material definition doesn't
 * already exist then create it first. Note that we don't write out the full
 * material properties (eg density), only the colour
 * @param objectName Name of object that material is to be assigned to
 * @param materialName Name of material to be assigned to the object
 * @param materialColor Colour of the material to be written into 3mf file
 */
void Mantid3MFFileIO::setMaterialOnObject(std::string objectName, std::string materialName, int materialColor) {

  Lib3MF_uint32 materialPropertyID;
  int baseMaterialsResourceID;
  AddBaseMaterial(materialName, materialColor, baseMaterialsResourceID, materialPropertyID);

  Lib3MF::PMeshObjectIterator meshObjectIterator = model->GetMeshObjects();
  bool meshObjectFound = false;
  while (meshObjectIterator->MoveNext()) {
    Lib3MF::PMeshObject meshObject = meshObjectIterator->GetCurrentMeshObject();
    // check if material already assigned to object
    if (meshObject->GetName() == objectName) {
      meshObjectFound = true;
      Lib3MF_uint32 resourceID, propertyID;
      bool propExists = meshObject->GetObjectLevelProperty(resourceID, propertyID);
      // currently just setObjectLevelProperty in all 3 branches of the
      // following so it's a bit verbose but different debug messages
      if (propExists) {
        Lib3MF::ePropertyType propType = model->GetPropertyTypeByID(resourceID);
        if (baseMaterialsResourceID == resourceID) {
          // update the material with the one in the input csv file
          g_log.debug("Existing material found for object " + objectName + ". Overwriting with material " +
                      materialName + "supplied in csv file");
          meshObject->SetObjectLevelProperty(baseMaterialsResourceID, materialPropertyID);
        } else {
          // if there's already a different property there (eg color) then
          // overwrite it
          g_log.debug("Existing non-material property found for object " + objectName +
                      ". Overwriting with material property with value " + materialName + " supplied in csv file");
          // clear properties first because lib3mf seems to write existing
          // color property down onto the individual triangles when you
          // overwrite the object level property. Don't want this
          meshObject->ClearAllProperties();
          meshObject->SetObjectLevelProperty(baseMaterialsResourceID, materialPropertyID);
        }
      } else {
        // add the material
        meshObject->SetObjectLevelProperty(baseMaterialsResourceID, materialPropertyID);
      }
    }
  }
  if (!meshObjectFound) {
    g_log.debug("Object " + objectName + " not found in 3MF file");
  }
}

/*
 * Write the 3mf data in the lib3mf model object out to a 3mf file
 * @param filename Name of the 3mf file to be created
 */
void Mantid3MFFileIO::saveFile(std::string filename) {
  Lib3MF::PWriter writer = model->QueryWriter("3mf");
  writer->WriteToFile(filename);
}

} // namespace DataHandling
} // namespace Mantid
