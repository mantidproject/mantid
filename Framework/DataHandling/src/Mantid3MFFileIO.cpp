// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/Mantid3MFFileIO.h"
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Matrix.h"
#include <boost/make_shared.hpp>
#include <iostream>

namespace {
constexpr auto SAMPLE_OBJECT_NAME = "sample";
constexpr double M_TOLERANCE = 0.000001;
} // namespace

namespace Mantid {
namespace DataHandling {

void Mantid3MFFileIO::LoadFile(std::string filename) {

  /*Assimp::Importer importer;

  const aiScene *pScene = importer.ReadFile(
      filename, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);*/

  // Import Model from 3MF File
  {
    Lib3MF::PReader reader = model->QueryReader("3mf");
    // And deactivate the strict mode (default is "false", anyway. This just
    // demonstrates where/how to use it).
    reader->SetStrictModeActive(false);
    reader->ReadFromFile(filename);

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
      throw "Units not supported";
    };
    setScaleType(scale);

    for (Lib3MF_uint32 iWarning = 0; iWarning < reader->GetWarningCount();
         iWarning++) {
      Lib3MF_uint32 nErrorCode;
      std::string sWarningMessage = reader->GetWarning(iWarning, nErrorCode);
      std::cout << "Encountered warning #" << nErrorCode << " : "
                << sWarningMessage << std::endl;
    }
  }
}

boost::shared_ptr<Geometry::MeshObject>
Mantid3MFFileIO::loadMeshObject(Lib3MF::PMeshObject meshObject,
                                sLib3MFTransform buildTransform) {

  Lib3MF_uint64 nVertexCount = meshObject->GetVertexCount();
  Lib3MF_uint64 nTriangleCount = meshObject->GetTriangleCount();

  g_log.debug("Mesh loaded");

  g_log.debug("Name: \"" + meshObject->GetName() + "\"");
  g_log.debug("PartNumber: \"" + meshObject->GetPartNumber() + "\"");
  g_log.debug("Vertex count: " + nVertexCount);
  g_log.debug("Triangle count: " + nTriangleCount);

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
    Mantid::Kernel::V3D vertex = createScaledV3D(
        i.m_Coordinates[0], i.m_Coordinates[1], i.m_Coordinates[2]);
    m_vertices.push_back(vertex);
  }

  Mantid::Kernel::Material material;

  Lib3MF_uint32 nResourceID;
  Lib3MF_uint32 nPropertyID;

  // load the material from the pid\pindex attributes on the mesh object
  if (meshObject->GetObjectLevelProperty(nResourceID, nPropertyID)) {

    Lib3MF::ePropertyType propType = model->GetPropertyTypeByID(nResourceID);

    if (propType == Lib3MF::ePropertyType::BaseMaterial) {

      Lib3MF::PBaseMaterialGroup baseMaterialGroup =
          model->GetBaseMaterialGroupByID(nResourceID);
      ReadMaterial::MaterialParameters params;
      std::string fullMaterialName = baseMaterialGroup->GetName(nPropertyID);
      std::string materialName;
      size_t openBracket = fullMaterialName.find("(");
      size_t closeBracket = fullMaterialName.find(")");
      if ((openBracket != std::string::npos) &&
          (closeBracket != std::string::npos)) {
        materialName = fullMaterialName.substr(0, openBracket);
        params.chemicalSymbol = materialName;
        std::string materialSpec = fullMaterialName.substr(
            openBracket + 1, closeBracket - openBracket - 1);
        std::stringstream materialSpecSS(materialSpec);
        std::string materialProperty;
        while (std::getline(materialSpecSS, materialProperty, ',')) {
          size_t equalsPos = materialProperty.find("=");
          if (equalsPos != std::string::npos) {
            std::string materialPropertyName =
                materialProperty.substr(0, equalsPos);
            std::string materialPropertyValue = materialProperty.substr(
                equalsPos + 1, materialProperty.length() - equalsPos - 1);
            double materialPropertyValueDouble;
            try {
              materialPropertyValueDouble = std::stoi(materialPropertyValue);
            } catch (std::exception) {
              g_log.debug("Unable to convert material property value + " +
                          materialPropertyValue + " to a double");
            }
            if (materialPropertyName == "MassDensity") {
              params.sampleMassDensity = materialPropertyValueDouble;
            }
          }
        }
        ReadMaterial reader;
        reader.setMaterialParameters(params);
        material = *(reader.buildMaterial());
      }
    }
  };

  auto mesh = boost::make_shared<Geometry::MeshObject>(
      std::move(m_triangle), std::move(m_vertices), material);
  mesh->setID(meshObject->GetName());

  // 3MF stores transformation matrix as row major so transpose
  // to store in Mantid Matrix class

  // scale the translation entries on 3rd row into metres
  Kernel::Matrix<double> transformMatrix(4, 4);
  transformMatrix[0][0] = buildTransform.m_Fields[0][0];
  transformMatrix[0][1] = buildTransform.m_Fields[1][0];
  transformMatrix[0][2] = buildTransform.m_Fields[2][0];
  transformMatrix[0][3] = scaleValue(buildTransform.m_Fields[3][0]);

  transformMatrix[1][0] = buildTransform.m_Fields[0][1];
  transformMatrix[1][1] = buildTransform.m_Fields[1][1];
  transformMatrix[1][2] = buildTransform.m_Fields[2][1];
  transformMatrix[1][3] = scaleValue(buildTransform.m_Fields[3][1]);

  transformMatrix[2][0] = buildTransform.m_Fields[0][2];
  transformMatrix[2][1] = buildTransform.m_Fields[1][2];
  transformMatrix[2][2] = buildTransform.m_Fields[2][2];
  transformMatrix[2][3] = scaleValue(buildTransform.m_Fields[3][2]);

  transformMatrix[3][0] = 0;
  transformMatrix[3][1] = 0;
  transformMatrix[3][2] = 0;
  transformMatrix[3][3] = 1;

  mesh->multiply(transformMatrix);

  return mesh;
}

void Mantid3MFFileIO::readMeshObjects(
    std::vector<boost::shared_ptr<Geometry::MeshObject>> &meshObjects,
    boost::shared_ptr<Geometry::MeshObject> &sample) {

  std::vector<boost::shared_ptr<Geometry::MeshObject>> retVal;
  Lib3MF::PBuildItemIterator buildItemIterator = model->GetBuildItems();
  while (buildItemIterator->MoveNext()) {
    Lib3MF::PBuildItem buildItem = buildItemIterator->GetCurrent();
    uint32_t objectResourceID = buildItem->GetObjectResourceID();
    // no general GetObjectByID in the lib3MF library??
    try {
      Lib3MF::PMeshObject meshObject =
          model->GetMeshObjectByID(objectResourceID);
      sLib3MFTransform transform = buildItem->GetObjectTransform();
      if (meshObject->GetName() == SAMPLE_OBJECT_NAME) {
        sample = loadMeshObject(meshObject, transform);
      } else {
        meshObjects.push_back(loadMeshObject(meshObject, transform));
      }
    } catch (std::exception) {
      Lib3MF::PComponentsObject componentsObject =
          model->GetComponentsObjectByID(objectResourceID);
      for (Lib3MF_uint32 nIndex = 0;
           nIndex < componentsObject->GetComponentCount(); nIndex++) {
        Lib3MF::PComponent component = componentsObject->GetComponent(nIndex);
        Lib3MF::PMeshObject meshObject =
            model->GetMeshObjectByID(component->GetObjectResourceID());
        sLib3MFTransform transform = buildItem->GetObjectTransform();
        if (meshObject->GetName() == SAMPLE_OBJECT_NAME) {
          sample = loadMeshObject(meshObject, {0});
        } else {
          meshObjects.push_back(loadMeshObject(meshObject, transform));
        }
      }
    }
  }
}

void Mantid3MFFileIO::writeMeshObject(
    const Geometry::MeshObject &mantidMeshObject, std::string name) {
  Lib3MF::PMeshObject meshObject = model->AddMeshObject();
  meshObject->SetName(name);
  std::vector<uint32_t> mantidTriangles = mantidMeshObject.getTriangles();
  std::vector<Kernel::V3D> mantidVertices = mantidMeshObject.getV3Ds();
  std::vector<sLib3MFTriangle> triangles;
  std::vector<sLib3MFPosition> vertices;

  // convert vertices from V3D to the Lib3MF struct
  for (auto i : mantidVertices) {
    sLib3MFPosition vertex = {{static_cast<Lib3MF_single>(i.X()),
                               static_cast<Lib3MF_single>(i.Y()),
                               static_cast<Lib3MF_single>(i.Z())}};
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
      g_log.debug("Face normal pointing to interior of object on object " +
                  mantidMeshObject.id() + ". Vertices swapped");
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

    AddBaseMaterial(mantidMeshObject.material().name(), generateRandomColor(),
                    baseMaterialsResourceID, materialPropertyID);
    meshObject->SetObjectLevelProperty(baseMaterialsResourceID,
                                       materialPropertyID);
  }

  // Set up one to one mapping between build items and mesh objects
  // Don't bother setting up any components
  // sLib3MFTransform mMatrix{0.0};
  sLib3MFTransform mMatrix = {
      {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}}};
  model->AddBuildItem(meshObject.get(), mMatrix);
}

int Mantid3MFFileIO ::generateRandomColor() {

  int rColor = (rand() % 256) << 16;
  int gColor = (rand() % 256) << 8;
  int bColor = (rand() % 256) << 0;

  return rColor | gColor | bColor;
  // return rand() % static_cast<int>(pow(16, 6));
}

// basic write to 3MF. Since Mantid is storing each instance of a shape
// as a separate mesh there's no possiblity of reusing meshes as supported
// by 3MF
void Mantid3MFFileIO::writeMeshObjects(
    std::vector<const Geometry::MeshObject *> meshObjects,
    const Geometry::MeshObject *sample, DataHandling::ScaleUnits scaleType) {

  // auto scaleType = getScaleType(scaleStr);
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

void Mantid3MFFileIO::AddBaseMaterial(std::string materialName,
                                      int materialColor, int &resourceID,
                                      Lib3MF_uint32 &materialPropertyID) {
  // check if master material definition exists
  bool materialNameExists = false;

  Lib3MF::PBaseMaterialGroupIterator materialIterator =
      model->GetBaseMaterialGroups();
  Lib3MF::PBaseMaterialGroup groupToAddTo;
  // Lib3MF_uint32 materialPropertyID;
  if (materialIterator->Count() == 0) {
    groupToAddTo = model->AddBaseMaterialGroup();
  } else {
    while (materialIterator->MoveNext() && !materialNameExists) {
      Lib3MF::PBaseMaterialGroup materialGroup =
          materialIterator->GetCurrentBaseMaterialGroup();
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
    materialPropertyID = groupToAddTo->AddMaterial(
        materialName, Lib3MF::sColor{rColor, gColor, bColor, 255});
  }
  resourceID = groupToAddTo->GetResourceID();
}

void Mantid3MFFileIO::setMaterialOnObject(std::string objectName,
                                          std::string materialName,
                                          int materialColor) {

  Lib3MF_uint32 materialPropertyID;
  int baseMaterialsResourceID;
  AddBaseMaterial(materialName, materialColor, baseMaterialsResourceID,
                  materialPropertyID);

  Lib3MF::PMeshObjectIterator meshObjectIterator = model->GetMeshObjects();
  bool meshObjectFound = false;
  while (meshObjectIterator->MoveNext()) {
    Lib3MF::PMeshObject meshObject = meshObjectIterator->GetCurrentMeshObject();
    // check if material already assigned to object
    if (meshObject->GetName() == objectName) {
      meshObjectFound = true;
      Lib3MF_uint32 resourceID, propertyID;
      bool propExists =
          meshObject->GetObjectLevelProperty(resourceID, propertyID);
      // currently just setObjectLevelProperty in all 3 branches of the
      // following so it's a bit verbose but different debug messages
      if (propExists) {
        Lib3MF::ePropertyType propType = model->GetPropertyTypeByID(resourceID);
        if (baseMaterialsResourceID == resourceID) {
          // update the material with the one in the input csv file
          g_log.debug("Existing material found for object " + objectName +
                      ". Overwriting with material " + materialName +
                      "supplied in csv file");
          meshObject->SetObjectLevelProperty(baseMaterialsResourceID,
                                             materialPropertyID);
        } else {
          // if there's already a different property there (eg color) then
          // overwrite it
          g_log.debug("Existing non-material property found for object " +
                      objectName +
                      ". Overwriting with material property with value " +
                      materialName + " supplied in csv file");
          // clear properties first because lib3mf seems to write existing
          // color property down onto the individual triangles when you
          // overwrite the object level property. Don't want this
          meshObject->ClearAllProperties();
          meshObject->SetObjectLevelProperty(baseMaterialsResourceID,
                                             materialPropertyID);
        }
      } else {
        // add the material
        meshObject->SetObjectLevelProperty(baseMaterialsResourceID,
                                           materialPropertyID);
      }
    }
  }
  if (!meshObjectFound) {
    g_log.debug("Object " + objectName + " not found in 3MF file");
  }
}

void Mantid3MFFileIO::ShowTransform(sLib3MFTransform transform,
                                    std::string indent) {
  std::cout << indent << "Transformation:  [ " << transform.m_Fields[0][0]
            << " " << transform.m_Fields[1][0] << " "
            << transform.m_Fields[2][0] << " " << transform.m_Fields[3][0]
            << " ]" << std::endl;
  std::cout << indent << "                 [ " << transform.m_Fields[0][1]
            << " " << transform.m_Fields[1][1] << " "
            << transform.m_Fields[2][1] << " " << transform.m_Fields[3][1]
            << " ]" << std::endl;
  std::cout << indent << "                 [ " << transform.m_Fields[0][2]
            << " " << transform.m_Fields[1][2] << " "
            << transform.m_Fields[2][2] << " " << transform.m_Fields[3][2]
            << " ]" << std::endl;
}

void Mantid3MFFileIO::ShowComponentsObjectInformation(
    Lib3MF::PComponentsObject componentsObject) {
  std::cout << "components object #" << componentsObject->GetResourceID()
            << ": " << std::endl;

  // ShowObjectProperties(componentsObject);
  std::cout << "   Component count:    "
            << componentsObject->GetComponentCount() << std::endl;
  for (Lib3MF_uint32 nIndex = 0; nIndex < componentsObject->GetComponentCount();
       nIndex++) {
    Lib3MF::PComponent component = componentsObject->GetComponent(nIndex);

    std::cout << "   Component " << nIndex
              << ":    Object ID:   " << component->GetObjectResourceID()
              << std::endl;
    if (component->HasTransform()) {
      ShowTransform(component->GetTransform(), "                   ");
    } else {
      std::cout << "                   Transformation:  none" << std::endl;
    }
  }
}

void Mantid3MFFileIO::ShowMetaDataInformation(
    Lib3MF::PMetaDataGroup metaDataGroup) {
  Lib3MF_uint32 nMetaDataCount = metaDataGroup->GetMetaDataCount();

  for (Lib3MF_uint32 iMeta = 0; iMeta < nMetaDataCount; iMeta++) {

    Lib3MF::PMetaData metaData = metaDataGroup->GetMetaData(iMeta);
    std::string sMetaDataValue = metaData->GetValue();
    std::string sMetaDataName = metaData->GetName();
    std::cout << "Metadatum: " << iMeta << ":" << std::endl;
    std::cout << "Name  = \"" << sMetaDataName << "\"" << std::endl;
    std::cout << "Value = \"" << sMetaDataValue << "\"" << std::endl;
  }
}

void Mantid3MFFileIO::saveFile(std::string filename) {
  Lib3MF::PWriter writer = model->QueryWriter("3mf");
  writer->WriteToFile(filename);
}

} // namespace DataHandling
} // namespace Mantid