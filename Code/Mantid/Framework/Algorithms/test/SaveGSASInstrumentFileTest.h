#ifndef MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_
#define MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SaveGSASInstrumentFile.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

using namespace std;

using Mantid::Algorithms::SaveGSASInstrumentFile;

class SaveGSASInstrumentFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveGSASInstrumentFileTest *createSuite() { return new SaveGSASInstrumentFileTest(); }
  static void destroySuite( SaveGSASInstrumentFileTest *suite ) { delete suite; }


  void test_SaveGSSInstrumentFile()
  {
    // Load a (local) table workspace
    loadProfileTable("PG3ProfileTable");
    TableWorkspace_sptr profiletablews = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("PG3ProfileTable"));
    TS_ASSERT(profiletablews);

    // Set up the algorithm
    SaveGSASInstrumentFile saver;
    saver.initialize();
    TS_ASSERT(saver.isInitialized());

    saver.setProperty("InputWorkspace", "PG3ProfileTable");
    saver.setProperty("OutputFilename", "test.iparm");
    saver.setPropertyValue("BankIDs", "4");
    saver.setProperty("Instrument", "PG3");
    saver.setPropertyValue("ChopperFrequency", "60");
    saver.setProperty("IDLine", "Blablabla Blablabla");
    saver.setProperty("Sample", "whatever");
    saver.setProperty("L1", 60.0);
    saver.setProperty("L2", 0.321);
    saver.setProperty("TwoTheta", 90.1);

    // Execute the algorithm
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check the output file against ... ....


    AnalysisDataService::Instance().remove("PG3ProfileTable");
    TS_ASSERT_EQUALS(1, 9876);

  }


  // Load table workspace containing instrument parameters
  void loadProfileTable(string wsname)
  {
    string tablewsname("pg3_bank1_params.nxs");

    LoadNexusProcessed loader;
    loader.initialize();

    loader.setProperty("Filename", tablewsname);
    loader.setProperty("OutputWorkspace", wsname);

    loader.execute();

    return;
  }

};


#endif /* MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_ */
