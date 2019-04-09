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

    TSM_ASSERT_EQUALS("should be 20 properties here", 20,
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
    double arrayToMatch[] = {0,  0, 0,  10, 10, 0,  10, 0,  0,  0, 10, 0,
                             10, 0, 30, 10, 10, 30, 0,  10, 30, 0, 0,  30};
    std::vector<double> vectorToMatch =
        std::vector<double>(std::begin(arrayToMatch), std::end(arrayToMatch));
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

  void testRotate() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("RotationMatrix", "0,-1,0,1,0,0,0,0,1");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    alg.rotate(environmentMesh);
    auto rotatedVertices = environmentMesh->getVertices();
    double arrayToMatch[] = {5, -5, -15, -5, 5, -15, 5,  5,  -15, -5, -5, -15,
                             5, 5,  15,  -5, 5, 15,  -5, -5, 15,  5,  -5, 15};
    std::vector<double> vectorToMatch =
        std::vector<double>(std::begin(arrayToMatch), std::end(arrayToMatch));
    TS_ASSERT(rotatedVertices == vectorToMatch);
  }

  void testRotateFailWrongSize() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("RotationMatrix", "-1,0,1,0,0,0,0,1");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    TS_ASSERT_THROWS(alg.rotate(environmentMesh), std::invalid_argument);
  }

  void testRotateFailInvalidMatrix() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("RotationMatrix", "6,1,1,4,-2,5,2,8,7");
    boost::shared_ptr<MeshObject> environmentMesh = nullptr;
    environmentMesh = loadCube();
    TS_ASSERT_THROWS(alg.rotate(environmentMesh), std::invalid_argument);
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
    const auto can = environment.getContainer();
    const auto &material = can->material();
    TSM_ASSERT_EQUALS(("expected elements"), environment.nelements(), 1);
    TS_ASSERT(can->hasValidShape());
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
        ws->sample().getEnvironment().getContainer()->material();
    TS_ASSERT_DELTA(material.numberDensity(), 0.23 * (2. + 3.), 1e-12);
  }

private:
  // load a cube into a meshobject
  std::unique_ptr<MeshObject> loadCube() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    auto loader = LoadBinaryStl(path);
    auto cube = loader.readStl();
    return cube;
  }
};

#endif /* LOAD_ENVIRONMENTTEST_H_ */
