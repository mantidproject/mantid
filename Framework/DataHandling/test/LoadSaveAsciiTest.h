#ifndef LOADSAVEASCIITEST_H_
#define LOADSAVEASCIITEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataHandling/SaveAscii.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Unit.h"
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadSaveAsciiTest : public CxxTest::TestSuite {

public:
  static LoadSaveAsciiTest *createSuite() { return new LoadSaveAsciiTest(); }
  static void destroySuite(LoadSaveAsciiTest *suite) { delete suite; }

  LoadSaveAsciiTest() {}
  ~LoadSaveAsciiTest() override {
    FrameworkManager::Instance().deleteWorkspace("LoadSaveAsciiWS_0");
    FrameworkManager::Instance().deleteWorkspace("LoadSaveAsciiWS_1");
  }

  void testSaveAndLoad() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
            WorkspaceFactory::Instance().create("Workspace2D", 9, 10, 10));
    for (int i = 0; i < 9; i++) {
      auto &X = wsToSave->mutableX(i);
      auto &Y = wsToSave->mutableY(i);
      auto &E = wsToSave->mutableE(i);
      for (int j = 0; j < 10; j++) {
        X[j] = 1. * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
      }
    }
    const std::string name = "LoadSaveAsciiWS_0";
    AnalysisDataService::Instance().add(name, wsToSave);

    std::string filename = "LoadSaveAsciiTestFile.dat";
    SaveAscii save;
    save.initialize();
    save.setPropertyValue("Filename", filename);
    filename = save.getPropertyValue("Filename"); // Get absolute path
    save.setPropertyValue("InputWorkspace", name);
    TS_ASSERT_THROWS_NOTHING(save.execute());

    LoadAscii load;
    load.initialize();
    load.setPropertyValue("Filename", filename);
    load.setPropertyValue("OutputWorkspace", "LoadSaveAsciiWS_1");
    TS_ASSERT_THROWS_NOTHING(load.execute());

    Workspace2D_sptr wsLoaded = boost::dynamic_pointer_cast<Workspace2D>(
        AnalysisDataService::Instance().retrieve("LoadSaveAsciiWS_1"));

    TS_ASSERT(wsLoaded);
    TS_ASSERT_EQUALS(wsLoaded->getNumberHistograms(), 9);
    TS_ASSERT_EQUALS(wsLoaded->blocksize(), 10);
    TS_ASSERT_EQUALS(wsLoaded->getAxis(0)->unit()->caption(), "Energy");
    TS_ASSERT_EQUALS(wsLoaded->getAxis(0)->unit()->label(), "meV");

    auto &X = wsLoaded->x(0);
    TS_ASSERT_EQUALS(X[0], 0);
    TS_ASSERT_EQUALS(X[1], 1.11111);
    TS_ASSERT_EQUALS(X[2], 2.22222);
    TS_ASSERT_EQUALS(X[5], 5.55556);

    TS_ASSERT_EQUALS(wsLoaded->y(0)[4], 19.7778);
    TS_ASSERT_EQUALS(wsLoaded->y(3)[7], 132.444);
    TS_ASSERT_EQUALS(wsLoaded->y(2)[5], 72.6667);
    TS_ASSERT_EQUALS(wsLoaded->y(5)[1], 38.6667);
    TS_ASSERT_EQUALS(wsLoaded->y(8)[8], 338);

    Poco::File(filename).remove();
  }
};

#endif /*LOADSAVEASCIITEST_H_*/
