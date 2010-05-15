#ifndef LOADSAVEASCIITEST_H_
#define LOADSAVEASCIITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataHandling/SaveAscii.h"
#include "Poco/File.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadSaveAsciiTest : public CxxTest::TestSuite
{

public:
  LoadSaveAsciiTest()
  {
    FrameworkManager::Instance();
//     Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<
//         Mantid::DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 9, 10, 10));
//     for (int i = 0; i < 9; i++)
//     {
//       std::vector<double>& X = ws->dataX(i);
//       std::vector<double>& Y = ws->dataY(i);
//       std::vector<double>& E = ws->dataE(i);
//       for (int j = 0; j < 10; j++)
//       {
//         X[j] = 1. * j / 0.9;
//         Y[j] = (i + 1) * (2. + 4. * X[j]);
//         E[j] = 1.;
//       }
//     }
//     AnalysisDataService::Instance().add("LoadSaveAsciiWS_0", ws);
  }
  ~LoadSaveAsciiTest()
  {
    //    FrameworkManager::Instance().deleteWorkspace("LoadSaveAsciiWS_0");
    //    FrameworkManager::Instance().deleteWorkspace("LoadSaveAsciiWS_1");
  }

  void testSaveAndLoad()
  {
    const std::string filename = "LoadSaveAsciiTestFile.dat";
//     SaveAscii save;
//     save.initialize();
//     save.setPropertyValue("Filename", filename);
//     save.setPropertyValue("Workspace", "LoadSaveAsciiWS_0");
//     TS_ASSERT_THROWS_NOTHING(save.execute());

//     LoadAscii load;
//     load.initialize();
//     load.setPropertyValue("Filename", filename);
//     load.setPropertyValue("Workspace", "LoadSaveAsciiWS_1");
//     TS_ASSERT_THROWS_NOTHING(load.execute());

//     Workspace2D_sptr ws = boost::dynamic_pointer_cast<Workspace2D>(
//         AnalysisDataService::Instance().retrieve("LoadSaveAsciiWS_1"));

//     TS_ASSERT(ws);
//     TS_ASSERT_EQUALS(ws->getNumberHistograms(), 9);
//     TS_ASSERT_EQUALS(ws->blocksize(), 10);
//     TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->caption(), "Energy");
//     TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->label(), "meV");

//     const std::vector<double>& X = ws->readX(0);
//     TS_ASSERT_EQUALS(X[0], 0);
//     TS_ASSERT_EQUALS(X[1], 1.11111);
//     TS_ASSERT_EQUALS(X[2], 2.22222);
//     TS_ASSERT_EQUALS(X[5], 5.55556);

//     TS_ASSERT_EQUALS(ws->readY(0)[4], 19.7778);
//     TS_ASSERT_EQUALS(ws->readY(3)[7], 132.444);
//     TS_ASSERT_EQUALS(ws->readY(2)[5], 72.6667);
//     TS_ASSERT_EQUALS(ws->readY(5)[1], 38.6667);
//     TS_ASSERT_EQUALS(ws->readY(8)[8], 338);

//    Poco::File(filename).remove();
  }
};


#endif /*LOADSAVEASCIITEST_H_*/
