// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOAD_ENVIRONMENTEST_H_
#define LOAD_ENVIRONMENTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadSampleEnvironment.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

class LoadSampleEnvironmentTest : public CxxTest::TestSuite {
public:
  static LoadSampleEnvironmentTest *createSuite() {
    return new LoadSampleEnvironmentTest();
  }
  static void destroySuite(LoadSampleEnvironmentTest *suite) { delete suite; }

  void testInit() {

    LoadSampleEnvironment alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TSM_ASSERT_EQUALS("should be 23 properties here", 23,
                      (size_t)(alg.getProperties().size()));
  }

  void testTranslate() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("TranslationVector", "5,5,15");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    alg.translate(environmentMesh);
    auto translatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {0,  0,  0,  10, 10, 0, 10, 0,
                                         0,  0,  10, 0,  10, 0, 30, 10,
                                         10, 30, 0,  10, 30, 0, 0,  30};
    TS_ASSERT(translatedVertices == vectorToMatch);
  }

  void testTranslateFailWrongSize() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("TranslationVector", "-1,0,1,0,0,0,0,1");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    TS_ASSERT_THROWS(alg.translate(environmentMesh), std::invalid_argument &);
  }

  void testGenerateXRotation() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("XDegrees", 90.0);
    auto rotationMatrix = alg.generateXRotation();
    std::vector<double> vectorToMatch = {1, 0, 0, 0, 0, -1, 0, 1, 0};
    compareMatrix(vectorToMatch, rotationMatrix);
  }

  void testGenerateYRotation() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("YDegrees", 90.0);
    auto rotationMatrix = alg.generateYRotation();
    std::vector<double> vectorToMatch = {0, 0, 1, 0, 1, 0, -1, 0, 0};
    compareMatrix(vectorToMatch, rotationMatrix);
  }

  void testGenerateZRotation() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("ZDegrees", 90.0);
    auto rotationMatrix = alg.generateZRotation();
    std::vector<double> vectorToMatch = {0, -1, 0, 1, 0, 0, 0, 0, 1};
    compareMatrix(vectorToMatch, rotationMatrix);
  }

  void testGenerateRotationMatrix() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("XDegrees", 90.0);
    alg.setProperty("YDegrees", 60.0);
    alg.setProperty("ZDegrees", 30.0);
    auto rotationMatrix = alg.generateMatrix();
    std::vector<double> vectorToMatch = {0.4330127, -0.2500000, 0.8660254,
                                         0.7500000, -0.4330127, -0.5000000,
                                         0.5000000, 0.8660254,  0.0000000};
    compareMatrix(vectorToMatch, rotationMatrix);
  }

  void testXRotation() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("XDegrees", "45");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    alg.rotate(environmentMesh);
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {
        -5, 7.07106,    -14.142136, 5,  14.142136,  -7.07106,
        5,  7.07106,    -14.142136, -5, 14.142136,  -7.07106,
        5,  -14.142136, 7.07106,    5,  -7.07106,   14.142136,
        -5, -7.07106,   14.142136,  -5, -14.142136, 7.07106};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }
  void testYRotation() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("YDegrees", "90");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    alg.rotate(environmentMesh);
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {-15, -5,  5,  -15, 5,  -5, -15, -5,
                                         -5,  -15, 5,  5,   15, -5, -5,  15,
                                         5,   -5,  15, 5,   5,  15, -5,  5};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

  void testZRotation() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("ZDegrees", "180");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    alg.rotate(environmentMesh);
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {5,   5,  -15, -5,  -5, -15, -5, 5,
                                         -15, 5,  -5,  -15, -5, 5,   15, -5,
                                         -5,  15, 5,   -5,  15, 5,   5,  15};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

  void testMultiRotation() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("XDegrees", "70");
    alg.setProperty("YDegrees", "20");
    alg.setProperty("ZDegrees", "35");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    alg.rotate(environmentMesh);
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {
        -6.28413,  10.46899,  -11.22095, -3.97647,  16.02167,  1.57914,
        1.41338,   15.06344,  -6.78932,  -11.67398, 11.42722,  -2.85248,
        11.673985, -11.42722, 2.85248,   6.28413,   -10.46899, 11.22095,
        -1.41338,  -15.06344, 6.78932,   3.97647,   -16.02167, -1.57914};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

  void testTranslateAndRotate() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("YDegrees", "90");
    alg.setProperty("TranslationVector", "0,0,15");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    alg.translate(environmentMesh);
    alg.rotate(environmentMesh);
    std::vector<double> rotatedVertices = environmentMesh->getVertices();
    std::vector<double> vectorToMatch = {0,  -5, 5,  0, 5,  -5, 0,  -5,
                                         -5, 0,  5,  5, 30, -5, -5, 30,
                                         5,  -5, 30, 5, 5,  30, -5, 5};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

  void testSetMaterial() {
    LoadSampleEnvironment alg;
    alg.initialize();
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    alg.setProperty("Filename", path);
    alg.setPropertyValue("EnvironmentName", "testName");
    alg.setProperty("SetMaterial", true);
    alg.setProperty("AtomicNumber", 1);
    alg.setProperty("MassNumber", 1);
    alg.setProperty("SampleNumberDensity", 1.0);
    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors,
                                                                     nbins);
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", "outputWorkspace");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");
    const auto &sample(ws->sample());
    const Geometry::SampleEnvironment environment = sample.getEnvironment();
    const auto &can = environment.getContainer();
    const auto &material = can.material();
    TSM_ASSERT_EQUALS(("expected elements"), environment.nelements(), 1);
    TS_ASSERT(can.hasValidShape());
    TS_ASSERT_EQUALS(environment.name(), "testName");
    TS_ASSERT_EQUALS(material.numberDensity(), 1);
    TS_ASSERT_EQUALS(material.name(), "");
  }

  void testSetMaterialNumberDensityInFormulaUnits() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    constexpr int nvectors{2}, nbins{10};
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors,
                                                                     nbins);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", path))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "outputWorkspace"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("EnvironmentName", "testName"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SetMaterial", true))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChemicalFormula", "Al2 O3"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SampleNumberDensity", 0.23))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("NumberDensityUnit", "Formula Units"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted());
    MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");
    const auto &material =
        ws->sample().getEnvironment().getContainer().material();
    TS_ASSERT_DELTA(material.numberDensity(), 0.23 * (2. + 3.), 1e-12);
  }

private:
  // load a cube into a meshobject
  std::unique_ptr<MeshObject> loadCube() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    constexpr ScaleUnits unit = ScaleUnits::metres;
    auto loader = LoadBinaryStl(path, unit);
    auto cube = loader.readStl();
    return cube;
  }

  void compareMatrix(const std::vector<double> &vectorToMatch,
                     const Kernel::Matrix<double> &rotationMatrix) {
    auto checkVector = rotationMatrix.getVector();
    for (size_t i = 0; i < 9; ++i) {
      TS_ASSERT_DELTA(checkVector[i], vectorToMatch[i], 1e-7);
    }
  }
};

#endif /* LOAD_ENVIRONMENTTEST_H_ */
