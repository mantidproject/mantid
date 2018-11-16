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
#include "MantidGeometry/Objects/MeshObject.h"
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

    TSM_ASSERT_EQUALS("should be 7 properties here", 7,
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

  void testRotate() {
    LoadSampleEnvironment alg;
    alg.initialize();
    alg.setProperty("rotationMatrix", "0,-1,0,1,0,0,0,0,1");
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

  std::unique_ptr<MeshObject> loadCube() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    auto loader = LoadBinaryStl(path);
    auto cube = loader.readStl();
    return cube;
  }
};

#endif /* LOAD_ENVIRONMENTTEST_H_ */