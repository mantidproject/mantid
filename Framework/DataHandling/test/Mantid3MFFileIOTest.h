// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/Mantid3MFFileIO.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

class Mantid3MFFileIOTest : public CxxTest::TestSuite {
public:
  static Mantid3MFFileIOTest *createSuite() { return new Mantid3MFFileIOTest(); }
  static void destroySuite(Mantid3MFFileIOTest *suite) { delete suite; }

  // perform some tests using 3mf file samples from the lib3mf consortium repo:
  // https://github.com/3MFConsortium/3mf-samples
  void testLoad() {
    std::string path = FileFinder::Instance().getFullPath("box.3mf").string();
    Mantid3MFFileIO MeshLoader;
    MeshLoader.LoadFile(path);
    std::vector<std::shared_ptr<Geometry::MeshObject>> environmentMeshes;
    std::shared_ptr<Geometry::MeshObject> sampleMesh;
    MeshLoader.readMeshObjects(environmentMeshes, sampleMesh);
    TS_ASSERT_EQUALS(sampleMesh, nullptr);
    TS_ASSERT_EQUALS(environmentMeshes.size(), 1);
    TS_ASSERT(environmentMeshes[0]->hasValidShape());
    TS_ASSERT_EQUALS(environmentMeshes[0]->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(environmentMeshes[0]->numberOfTriangles(), 12);
    TS_ASSERT_EQUALS(MeshLoader.getScaleType(), ScaleUnits::millimetres);
  }

  void testLoadSample() {
    std::string path = FileFinder::Instance().getFullPath("box_sample.3mf").string();
    Mantid3MFFileIO MeshLoader;
    MeshLoader.LoadFile(path);
    std::vector<std::shared_ptr<Geometry::MeshObject>> environmentMeshes;
    std::shared_ptr<Geometry::MeshObject> sampleMesh;
    MeshLoader.readMeshObjects(environmentMeshes, sampleMesh);
    TS_ASSERT_EQUALS(environmentMeshes.size(), 0);
    TS_ASSERT(sampleMesh->hasValidShape());
    TS_ASSERT_EQUALS(sampleMesh->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(sampleMesh->numberOfTriangles(), 12);
    TS_ASSERT_EQUALS(MeshLoader.getScaleType(), ScaleUnits::millimetres);
  }

  void testLoadWithMaterial() {
    std::string path = FileFinder::Instance().getFullPath("box_withMaterial.3mf").string();
    Mantid3MFFileIO MeshLoader;
    MeshLoader.LoadFile(path);
    std::vector<std::shared_ptr<Geometry::MeshObject>> environmentMeshes;
    std::shared_ptr<Geometry::MeshObject> sampleMesh;
    MeshLoader.readMeshObjects(environmentMeshes, sampleMesh);
    TS_ASSERT_EQUALS(sampleMesh, nullptr);
    TS_ASSERT_EQUALS(environmentMeshes.size(), 1);
    TS_ASSERT(environmentMeshes[0]->hasValidShape());
    TS_ASSERT_EQUALS(environmentMeshes[0]->material().name(), "B4-C");
  }

  void testLoadWithInvalidMaterial() {
    std::string path = FileFinder::Instance().getFullPath("box_withInvalidMaterial.3mf").string();
    Mantid3MFFileIO MeshLoader;
    MeshLoader.LoadFile(path);
    std::vector<std::shared_ptr<Geometry::MeshObject>> environmentMeshes;
    std::shared_ptr<Geometry::MeshObject> sampleMesh;
    MeshLoader.readMeshObjects(environmentMeshes, sampleMesh);
    // exception is caught and mesh loaded without material data
    TS_ASSERT_EQUALS(environmentMeshes[0]->material().name(), "");
  }

  void testLoadWithInvalidUnits() {
    std::string path = FileFinder::Instance().getFullPath("box_withInvalidUnits.3mf").string();
    Mantid3MFFileIO MeshLoader;
    MeshLoader.LoadFile(path);
    std::vector<std::shared_ptr<Geometry::MeshObject>> environmentMeshes;
    std::shared_ptr<Geometry::MeshObject> sampleMesh;
    // invalid units logged as warning by Lib3MF reader and units default to mm
    TS_ASSERT_THROWS_NOTHING(MeshLoader.readMeshObjects(environmentMeshes, sampleMesh));
    TS_ASSERT_EQUALS(MeshLoader.getScaleType(), ScaleUnits::millimetres);
  }

  void testLoadMultipleObjects() {
    std::string path = FileFinder::Instance().getFullPath("multiple_cylinders.3mf").string();
    Mantid3MFFileIO MeshLoader;
    MeshLoader.LoadFile(path);
    std::vector<std::shared_ptr<Geometry::MeshObject>> environmentMeshes;
    std::shared_ptr<Geometry::MeshObject> sampleMesh;
    MeshLoader.readMeshObjects(environmentMeshes, sampleMesh);
    TS_ASSERT_EQUALS(sampleMesh, nullptr);
    TS_ASSERT_EQUALS(environmentMeshes.size(), 6);
    for (auto shape : environmentMeshes) {
      TS_ASSERT(shape->hasValidShape());
    }
    TS_ASSERT_EQUALS(environmentMeshes[0]->numberOfVertices(), 46);
    TS_ASSERT_EQUALS(environmentMeshes[0]->numberOfTriangles(), 88);
    // Mantid ignores material definitions unless properties are also supplied
    TS_ASSERT_EQUALS(environmentMeshes[0]->material().name(), "");
  }

  void testLoadBigFile() {
    std::string path = FileFinder::Instance().getFullPath("T-Rex.3mf").string();
    Mantid3MFFileIO MeshLoader;
    MeshLoader.LoadFile(path);
    std::vector<std::shared_ptr<Geometry::MeshObject>> environmentMeshes;
    std::shared_ptr<Geometry::MeshObject> sampleMesh;
    MeshLoader.readMeshObjects(environmentMeshes, sampleMesh);
    TS_ASSERT_EQUALS(sampleMesh, nullptr);
    TS_ASSERT_EQUALS(environmentMeshes.size(), 3);
    for (auto shape : environmentMeshes) {
      TS_ASSERT(shape->hasValidShape());
    }
    TS_ASSERT_EQUALS(environmentMeshes[0]->numberOfVertices(), 6895);
    TS_ASSERT_EQUALS(environmentMeshes[0]->numberOfTriangles(), 13794);
  }
};
