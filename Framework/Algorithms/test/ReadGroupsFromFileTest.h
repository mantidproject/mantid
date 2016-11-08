#ifndef READGROUPSFROMFILETEST_H_
#define READGROUPSFROMFILETEST_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CreateGroupingWorkspace.h"
#include "MantidAlgorithms/ReadGroupsFromFile.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/SaveCalFile.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include <Poco/File.h>
#include <cstring>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class ReadGroupsFromFileTest : public CxxTest::TestSuite {
public:
  void testINES() {
    LoadEmptyInstrument loaderCAL;

    loaderCAL.initialize();
    loaderCAL.isInitialized();
    loaderCAL.setPropertyValue(
        "Filename",
        ConfigService::Instance().getString("instrumentDefinition.directory") +
            "/INES_Definition.xml");
    const std::string wsName = "LoadEmptyInstrumentTestCAL";
    loaderCAL.setPropertyValue("OutputWorkspace", wsName);
    loaderCAL.execute();
    loaderCAL.isExecuted();

    CreateGroupingWorkspace testerCAL;

    TS_ASSERT_THROWS_NOTHING(testerCAL.initialize());
    TS_ASSERT_THROWS_NOTHING(testerCAL.isInitialized());
    testerCAL.setPropertyValue("InstrumentName", "INES");
    testerCAL.setPropertyValue("OutputWorkspace", "grp");
    testerCAL.setPropertyValue(
        "GroupNames",
        "bank1A,bank2B,bank3C,bank4D,bank5E,bank6F,bank7G,bank8H,bank9I");

    TS_ASSERT_THROWS_NOTHING(testerCAL.execute());
    TS_ASSERT(testerCAL.isExecuted());

    // has the algorithm written a file to disk?

    GroupingWorkspace_sptr groupWS =
        AnalysisDataService::Instance().retrieveWS<GroupingWorkspace>("grp");
    std::string outputFile = "./INES_DspacemaptoCalTest.cal";

    SaveCalFile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", groupWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outputFile));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    outputFile = alg.getPropertyValue("Filename");

    bool fileExists = false;
    TS_ASSERT(fileExists = Poco::File(outputFile).exists());

    if (fileExists) {

      ReadGroupsFromFile alg;
      alg.initialize();
      alg.setPropertyValue("InstrumentWorkspace", wsName);
      alg.setPropertyValue("GroupingFileName", outputFile);
      alg.setPropertyValue("OutputWorkspace",
                           "ReadGroupsFromFileTest_Workspace");

      TS_ASSERT_THROWS_NOTHING(alg.execute());
      TS_ASSERT(alg.isExecuted());

      Mantid::API::MatrixWorkspace_const_sptr ws;
      TS_ASSERT_THROWS_NOTHING(
          ws = boost::dynamic_pointer_cast<const MatrixWorkspace>(
              AnalysisDataService::Instance().retrieve(
                  "ReadGroupsFromFileTest_Workspace")););
      TS_ASSERT(ws);

      TS_ASSERT_EQUALS(ws->blocksize(), 1);

      TS_ASSERT_DELTA(ws->y(2)[0], 1.0, 1e-6);
      TS_ASSERT_DELTA(ws->y(25)[0], 2.0, 1e-6);
      TS_ASSERT_DELTA(ws->y(45)[0], 3.0, 1e-6);

      // remove file created by this algorithm
      Poco::File(outputFile).remove();
    }

    // Remove workspace
    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif
