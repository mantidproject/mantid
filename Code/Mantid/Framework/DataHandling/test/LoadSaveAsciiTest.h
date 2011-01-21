#ifndef LOADSAVEASCIITEST_H_
#define LOADSAVEASCIITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataHandling/SaveAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "Poco/File.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadSaveAsciiTest : public CxxTest::TestSuite
{

public:

    static LoadSaveAsciiTest *createSuite() { return new LoadSaveAsciiTest(); }
  static void destroySuite(LoadSaveAsciiTest *suite) { delete suite; }

  LoadSaveAsciiTest()
  {
     
  }
  ~LoadSaveAsciiTest()
  {
    FrameworkManager::Instance().deleteWorkspace("LoadSaveAsciiWS_0");
    FrameworkManager::Instance().deleteWorkspace("LoadSaveAsciiWS_1");
  }

  void testSaveAndLoad()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave = boost::dynamic_pointer_cast<
         Mantid::DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 9, 10, 10));
     for (int i = 0; i < 9; i++)
     {
       std::vector<double>& X = wsToSave->dataX(i);
       std::vector<double>& Y = wsToSave->dataY(i);
       std::vector<double>& E = wsToSave->dataE(i);
       for (int j = 0; j < 10; j++)
       {
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
     filename = save.getPropertyValue("Filename"); //Get absolute path
     save.setPropertyValue("Workspace", name);
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

     const std::vector<double>& X = wsLoaded->readX(0);
     TS_ASSERT_EQUALS(X[0], 0);
     TS_ASSERT_EQUALS(X[1], 1.11111);
     TS_ASSERT_EQUALS(X[2], 2.22222);
     TS_ASSERT_EQUALS(X[5], 5.55556);

     TS_ASSERT_EQUALS(wsLoaded->readY(0)[4], 19.7778);
     TS_ASSERT_EQUALS(wsLoaded->readY(3)[7], 132.444);
     TS_ASSERT_EQUALS(wsLoaded->readY(2)[5], 72.6667);
     TS_ASSERT_EQUALS(wsLoaded->readY(5)[1], 38.6667);
     TS_ASSERT_EQUALS(wsLoaded->readY(8)[8], 338);

    Poco::File(filename).remove();
  }
};


#endif /*LOADSAVEASCIITEST_H_*/
