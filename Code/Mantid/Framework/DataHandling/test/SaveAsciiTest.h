#ifndef SAVEASCIITEST_H_
#define SAVEASCIITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataHandling/SaveAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FrameworkManager.h"
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveAsciiTest : public CxxTest::TestSuite
{

public:

  static SaveAsciiTest *createSuite() { return new SaveAsciiTest(); }
  static void destroySuite(SaveAsciiTest *suite) { delete suite; }

  SaveAsciiTest()
  {

  }
  ~SaveAsciiTest()
  {
    FrameworkManager::Instance().deleteWorkspace("SaveAsciiWS_0");
  }

  void testSave()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave = boost::dynamic_pointer_cast<
      Mantid::DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 4, 5, 5));
    for (int i = 0; i < 4; i++)
    {
      std::vector<double>& X = wsToSave->dataX(i);
      std::vector<double>& Y = wsToSave->dataY(i);
      std::vector<double>& E = wsToSave->dataE(i);
      for (int j = 0; j < 5; j++)
      {
        X[j] = 1. * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
      }
    }
    const std::string name = "SaveAsciiWS_0";
    AnalysisDataService::Instance().add(name, wsToSave);

    std::string filename = "SaveAsciiTestFile.dat";
    SaveAscii save;
    save.initialize();
    save.setPropertyValue("Filename", filename);
    filename = save.getPropertyValue("Filename"); //Get absolute path
    save.setPropertyValue("InputWorkspace", name);
    TS_ASSERT_THROWS_NOTHING(save.execute());


    Poco::File(filename).remove();
  }
};


#endif /*SAVEASCIITEST_H_*/
