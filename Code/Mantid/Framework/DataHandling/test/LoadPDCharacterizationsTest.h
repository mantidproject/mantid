#ifndef MANTID_DATAHANDLING_LOADPDCHARACTERIZATIONSTEST_H_
#define MANTID_DATAHANDLING_LOADPDCHARACTERIZATIONSTEST_H_

#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadPDCharacterizations.h"
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;

using Mantid::DataHandling::LoadPDCharacterizations;
class LoadPDCharacterizationsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadPDCharacterizationsTest *createSuite() { return new LoadPDCharacterizationsTest(); }
  static void destroySuite( LoadPDCharacterizationsTest *suite ) { delete suite; }


  void test_Init()
  {
    LoadPDCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
//  Test/AutoTestData/Test_characterizations_char.txt
//  Test/AutoTestData/Test_characterizations_focus.txt
//  Test/AutoTestData/Test_characterizations_focus_and_char.txt
  void test_FocusAndChar()
  {
    std::string CHAR_FILE("Test_characterizations_focus_and_char.txt");

    // initialize the algorithm
    LoadPDCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    // run the algorithm
    alg.setProperty("Filename", CHAR_FILE);
    alg.setPropertyValue("OutputWorkspace", CHAR_FILE);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // test the table workspace
    ITableWorkspace_sptr wksp = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
        (Mantid::API::AnalysisDataService::Instance().retrieve(CHAR_FILE));
    TS_ASSERT(wksp);
    TS_ASSERT_EQUALS(wksp->columnCount(), 10);
    TS_ASSERT_EQUALS(wksp->rowCount(), 6);

    // check all of the contents of row 0
    TS_ASSERT_EQUALS(wksp->Double(0,0), 60.);
    TS_ASSERT_EQUALS(wksp->Double(0,1), 0.900);
    TS_ASSERT_EQUALS(wksp->Int(0,2), 1);
    TS_ASSERT_EQUALS(wksp->Int(0,3), 15030);
    TS_ASSERT_EQUALS(wksp->Int(0,4), 15039);
    TS_ASSERT_EQUALS(wksp->Int(0,5), 0);
    TS_ASSERT_EQUALS(wksp->Double(0,6), 0.20);
    TS_ASSERT_EQUALS(wksp->Double(0,7), 4.12);
    TS_ASSERT_EQUALS(wksp->Double(0,8), 4700.);
    TS_ASSERT_EQUALS(wksp->Double(0,9), 21200.);

    // check all of the contents of row 5
    TS_ASSERT_EQUALS(wksp->Double(5,0), 10.);
    TS_ASSERT_EQUALS(wksp->Double(5,1), 3.198);
    TS_ASSERT_EQUALS(wksp->Int(5,2), 1);
    TS_ASSERT_EQUALS(wksp->Int(5,3), 15033);
    TS_ASSERT_EQUALS(wksp->Int(5,4), 15042);
    TS_ASSERT_EQUALS(wksp->Int(5,5), 0);
    TS_ASSERT_EQUALS(wksp->Double(5,6), .05);
    TS_ASSERT_EQUALS(wksp->Double(5,7), 15.40);
    TS_ASSERT_EQUALS(wksp->Double(5,8), 0.);
    TS_ASSERT_EQUALS(wksp->Double(5,9), 100000.);

    // test the other output properties
    TS_ASSERT_EQUALS(alg.getPropertyValue("IParmFilename"), std::string("dummy.iparm"));
    double l1 = alg.getProperty("PrimaryFlightPath");
    TS_ASSERT_EQUALS(l1, 60.);

    std::vector<int32_t> specIds = alg.getProperty("SpectrumIDs");
    TS_ASSERT_EQUALS(specIds.size(), 1);
    if (!specIds.empty())
      TS_ASSERT_EQUALS(specIds[0], 1);

    std::vector<double> l2 = alg.getProperty("L2");
    TS_ASSERT_EQUALS(l2.size(), 1);
    if (!l2.empty())
      TS_ASSERT_EQUALS(l2[0], 3.18);

    std::vector<double> polar = alg.getProperty("Polar");
    TS_ASSERT_EQUALS(polar.size(), 1);
    if (!polar.empty())
      TS_ASSERT_EQUALS(polar[0], 90.);

    std::vector<double> azi = alg.getProperty("Azimuthal");
    TS_ASSERT_EQUALS(azi.size(), 1);
    if (!azi.empty())
      TS_ASSERT_EQUALS(azi[0], 0.);
  }

//  void test_exec()
//  {
//    // Name of the output workspace.
//    std::string outWSName("LoadPDCharacterizationsTest_OutputWS");
  
//    LoadPDCharacterizations alg;
//    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
//    TS_ASSERT( alg.isInitialized() )
//    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value") );
//    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
//    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
//    TS_ASSERT( alg.isExecuted() );
    
//    // Retrieve the workspace from data service. TODO: Change to your desired type
//    Workspace_sptr ws;
//    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
//    TS_ASSERT(ws);
//    if (!ws) return;
    
//    // TODO: Check the results
    
//    // Remove workspace from the data service.
//    AnalysisDataService::Instance().remove(outWSName);
//  }
  
//  void test_Something()
//  {
//    TSM_ASSERT( "You forgot to write a test!", 0);
//  }


};


#endif /* MANTID_DATAHANDLING_LOADPDCHARACTERIZATIONSTEST_H_ */
