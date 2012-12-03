#ifndef SAVEASCIITEST_H_
#define SAVEASCIITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FrameworkManager.h"
#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

// This test tests that SaveAscii produces a file of the expected form
// It does not test that the file can be loaded by loadAscii.
// The test LoadSaveAscii does that and should be run in addition to this test
// if you modify SaveAscii.

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
    FrameworkManager::Instance().deleteWorkspace("SaveAsciiWS");
  }

  void testExec()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave = boost::dynamic_pointer_cast<
      Mantid::DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++)
    {
      std::vector<double>& X = wsToSave->dataX(i);
      std::vector<double>& Y = wsToSave->dataY(i);
      std::vector<double>& E = wsToSave->dataE(i);
      for (int j = 0; j < 3; j++)
      {
        X[j] = 1. * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
      }
    }
    const std::string name = "SaveAsciiWS";
    AnalysisDataService::Instance().add(name, wsToSave);

    std::string filename = "SaveAsciiTestFile.dat";
    std::string filename_nohead ="SaveAsciiTestFileWithoutHeader.dat";

    SaveAscii save;
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT( save.isInitialized() )
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", filename));
    filename = save.getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("InputWorkspace", name));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str()); 

    // Currently we just test that the first few column headers are as expected
    in.close();


    // Test ColumnHeader property - perhaps this should be a separate test
    save.setPropertyValue("Filename", filename_nohead);
    save.setPropertyValue("InputWorkspace", name);
    TS_ASSERT_THROWS_NOTHING(save.setProperty("ColumnHeader", false));
    filename_nohead = save.getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename_nohead).exists() );


    // Remove files
    Poco::File(filename).remove();
    Poco::File(filename_nohead).remove();
  }
};


#endif /*SAVEASCIITEST_H_*/
