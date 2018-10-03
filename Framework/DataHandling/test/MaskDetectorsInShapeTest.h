// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MARKDEADDETECTORSINSHAPETEST_H_
#define MARKDEADDETECTORSINSHAPETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/MaskDetectorsInShape.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::API;

class MaskDetectorsInShapeTest : public CxxTest::TestSuite {
public:
  static MaskDetectorsInShapeTest *createSuite() {
    return new MaskDetectorsInShapeTest();
  }
  static void destroySuite(MaskDetectorsInShapeTest *suite) { delete suite; }

  MaskDetectorsInShapeTest() { loadTestWS(); }

  ~MaskDetectorsInShapeTest() override {
    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void testCuboidMiss() {
    std::string xmlShape = "<cuboid id=\"shape\"> ";
    xmlShape += R"(<left-front-bottom-point x="0.005" y="-0.1" z="0.0" /> )";
    xmlShape +=
        R"(<left-front-top-point x="0.005" y="-0.1" z="0.0001" />  )";
    xmlShape +=
        R"(<left-back-bottom-point x="-0.005" y="-0.1" z="0.0" />  )";
    xmlShape +=
        R"(<right-front-bottom-point x="0.005" y="0.1" z="0.0" />  )";
    xmlShape += "</cuboid> ";
    xmlShape += "<algebra val=\"shape\" /> ";

    runTest(xmlShape, "");
  }

  void testConeHitNoMonitors() {
    // algebra line is essential
    std::string xmlShape = "<cone id=\"shape\"> ";
    xmlShape += R"(<tip-point x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="0.0" z="-1" /> )";
    xmlShape += "<angle val=\"8.1\" /> ";
    xmlShape += "<height val=\"4\" /> ";
    xmlShape += "</cone>";
    xmlShape += "<algebra val=\"shape\" /> ";

    runTest(xmlShape, "320,340,360,380", false);
  }

  void runTest(std::string xmlShape, std::string expectedHits,
               bool includeMonitors = true) {
    using namespace Mantid::API;

    Mantid::DataHandling::MaskDetectorsInShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("Workspace", wsName);
    alg.setPropertyValue("ShapeXML", xmlShape);
    if (includeMonitors) {
      alg.setPropertyValue("IncludeMonitors", "1");
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_const_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

    checkDeadDetectors(outWS, expectedHits);
  }

  void checkDeadDetectors(Mantid::API::MatrixWorkspace_const_sptr outWS,
                          std::string expectedHits) {
    // check that the detectors have actually been marked dead
    std::vector<int> expectedDetectorArray =
        convertStringToVector(expectedHits);
    const auto &detectorInfo = outWS->detectorInfo();
    for (const auto detID : expectedDetectorArray) {
      TS_ASSERT(detectorInfo.isMasked(detectorInfo.indexOf(detID)));
    }
  }

  std::vector<int> convertStringToVector(const std::string input) {
    Mantid::Kernel::ArrayProperty<int> arrayProp("name", input);
    return arrayProp();
  }

  std::string loadTestWS() {
    Mantid::DataHandling::LoadEmptyInstrument loaderSLS;

    TS_ASSERT_THROWS_NOTHING(loaderSLS.initialize());
    TS_ASSERT(loaderSLS.isInitialized());
    inputFile = "SANDALS_Definition.xml";
    loaderSLS.setPropertyValue("Filename", inputFile);
    wsName = "MaskDetectorsInShapeTest_MaskDetectorsInShapeTestSLS";
    loaderSLS.setPropertyValue("OutputWorkspace", wsName);

    TS_ASSERT_THROWS_NOTHING(loaderSLS.execute());
    TS_ASSERT(loaderSLS.isExecuted());

    return wsName;
  }

private:
  std::string inputFile;
  std::string wsName;
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class MaskDetectorsInShapeTestPerformance : public CxxTest::TestSuite {

private:
  const std::string workspace;

public:
  static MaskDetectorsInShapeTestPerformance *createSuite() {
    return new MaskDetectorsInShapeTestPerformance();
  }
  static void destroySuite(MaskDetectorsInShapeTestPerformance *suite) {
    delete suite;
  }

  MaskDetectorsInShapeTestPerformance() : workspace("SANS2D") {
    // Load the instrument alone so as to isolate the raw file loading time from
    // the instrument loading time
    IAlgorithm *loader =
        FrameworkManager::Instance().createAlgorithm("LoadEmptyInstrument");
    loader->setPropertyValue("Filename", "SANS2D_Definition.xml");
    loader->setPropertyValue("OutputWorkspace", workspace);
    TS_ASSERT(loader->execute());
  }

  void testMaskingLotsOfDetectors() {
    IAlgorithm *masker =
        FrameworkManager::Instance().createAlgorithm("MaskDetectorsInShape");
    masker->setPropertyValue("Workspace", workspace);
    masker->setPropertyValue(
        "ShapeXML", "<infinite-cylinder id=\"beam_area\"><centre x=\"0\" "
                    "y=\"0\" z=\"0.0\" /><axis x=\"0\" y=\"0\" z=\"1\" "
                    "/><radius val=\"0.28\" /></infinite-cylinder><algebra "
                    "val=\"#beam_area\"/>");
    masker->execute();

    AnalysisDataService::Instance().remove(workspace);
  }
};
#endif /*MARKDEADDETECTORSINSHAPETEST_H_*/
