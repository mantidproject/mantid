// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/MeshFileIO.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

class MeshFileIOTest : public CxxTest::TestSuite {
public:
  static MeshFileIOTest *createSuite() { return new MeshFileIOTest(); }
  static void destroySuite(MeshFileIOTest *suite) { delete suite; }

  void testTranslate() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    auto loader = LoadBinaryStl(path, unit);
    std::shared_ptr<MeshObject> environmentMesh = loader.readShape();
    loader.translate(environmentMesh, {5, 5, 15});
    auto translatedVertices = environmentMesh->getVertices();

    std::vector<double> vectorToMatch = {0,  0, 0,  10, 10, 0,  10, 0,  0,  0, 10, 0,
                                         10, 0, 30, 10, 10, 30, 0,  10, 30, 0, 0,  30};
    TS_ASSERT(translatedVertices == vectorToMatch);
  }

  void testTranslateFailWrongSize() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    auto loader = LoadBinaryStl(path, unit);
    std::shared_ptr<MeshObject> environmentMesh = loader.readShape();
    TS_ASSERT_THROWS(loader.translate(environmentMesh, {-1, 0, 1, 0, 0, 0, 0, 1}), const std::invalid_argument &);
  }

  void testXRotation() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    auto loader = LoadBinaryStl(path, unit);
    std::shared_ptr<MeshObject> environmentMesh = loader.readShape();
    loader.rotate(environmentMesh, 45 * M_PI / 180, 0, 0);
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {-5, 7.07106,   -14.142136, 5,  14.142136,  -7.07106, 5, 7.07106,  -14.142136,
                                         -5, 14.142136, -7.07106,   5,  -14.142136, 7.07106,  5, -7.07106, 14.142136,
                                         -5, -7.07106,  14.142136,  -5, -14.142136, 7.07106};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }
  void testYRotation() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    auto loader = LoadBinaryStl(path, unit);
    std::shared_ptr<MeshObject> environmentMesh = loader.readShape();
    loader.rotate(environmentMesh, 0, 90 * M_PI / 180, 0);
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {-15, -5, 5,  -15, 5, -5, -15, -5, -5, -15, 5,  5,
                                         15,  -5, -5, 15,  5, -5, 15,  5,  5,  15,  -5, 5};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

  void testZRotation() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    auto loader = LoadBinaryStl(path, unit);
    std::shared_ptr<MeshObject> environmentMesh = loader.readShape();
    loader.rotate(environmentMesh, 0, 0, 180 * M_PI / 180);
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {5,  5, -15, -5, -5, -15, -5, 5,  -15, 5, -5, -15,
                                         -5, 5, 15,  -5, -5, 15,  5,  -5, 15,  5, 5,  15};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

  void testMultiRotation() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    auto loader = LoadBinaryStl(path, unit);
    std::shared_ptr<MeshObject> environmentMesh = loader.readShape();
    loader.rotate(environmentMesh, 70 * M_PI / 180, 20 * M_PI / 180, 35 * M_PI / 180);
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {-13.70635, 5.52235,   -7.52591,  -5.33788,  15.55731,  -2.11589,
                                         -6.00884,  10.91220,  -10.94611, -13.03539, 10.16745,  1.30430,
                                         13.03539,  -10.16745, -1.30430,  13.70635,  -5.52235,  7.52591,
                                         6.00884,   -10.91220, 10.94611,  5.33788,   -15.55731, 2.11589};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

  void testTranslateAndRotate() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl").string();
    auto loader = LoadBinaryStl(path, unit);
    std::shared_ptr<MeshObject> environmentMesh = loader.readShape();
    loader.rotate(environmentMesh, 0, 90 * M_PI / 180, 0);
    loader.translate(environmentMesh, {0, 0, 15});
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {-15, -5, 20, -15, 5, 10, -15, -5, 10, -15, 5,  20,
                                         15,  -5, 10, 15,  5, 10, 15,  5,  20, 15,  -5, 20};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

private:
  const ScaleUnits unit = ScaleUnits::metres;
};
