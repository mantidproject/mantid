#ifndef RAWFILEINFOTEST_H_
#define RAWFILEINFOTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/RawFileInfo.h"

#include <iostream>
#include <iomanip>

using namespace Mantid::DataHandling;


class RawFileInfoTest : public CxxTest::TestSuite
{
  
public:
  
  // Perform test with a GEM file
  RawFileInfoTest() : m_filetotest("../../../../Test/AutoTestData/LOQ48127.raw") 
  {
  }
  
  // Test output parameters without table workspace output
  void testNoRunParameters()
  {
    runTest(false);
  }

  void testGetRunParameters()
  {
    runTest(true);
  }
  

private:

  // Check the parameters are correct
  void runTest(bool tableToExist)
  {
    RawFileInfo alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_EQUALS(alg.isInitialized(), true);

    // Set the file name
    alg.setPropertyValue("Filename", m_filetotest);
    if( tableToExist )
    {
      alg.setPropertyValue("GetRunParameters", "1");
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT_EQUALS(alg.isExecuted(), true);

    // Check the output parameters are what we expect
    std::string title = alg.getProperty("RunTitle");
    TS_ASSERT_EQUALS(title, std::string("direct beam                                                                     "));
    std::string header = alg.getProperty("RunHeader");
    TS_ASSERT_EQUALS(header, std::string("48127 LOQ team & SANS Xpre direct beam              18-DEC-2008 17:58:38"));
    int spectra_count = alg.getProperty("SpectraCount"); //7290
    TS_ASSERT_EQUALS(spectra_count, 8);
      
    int bin_count = alg.getProperty("TimeChannelCount"); //5050
    TS_ASSERT_EQUALS(bin_count, 102);

    int prd_count = alg.getProperty("PeriodCount");
    TS_ASSERT_EQUALS(prd_count, 1);

    //Finally test that a workspace existence is correct
    TS_ASSERT_EQUALS( Mantid::API::AnalysisDataService::Instance().doesExist("Raw_RPB"), tableToExist );
    
    if( tableToExist )
    {
      Mantid::API::Workspace_sptr workspace = Mantid::API::AnalysisDataService::Instance().retrieve("Raw_RPB");
      TS_ASSERT( workspace.get() );

      Mantid::API::ITableWorkspace_sptr run_table = 
	boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(workspace);
      TS_ASSERT( run_table.get() );
	
      //Check a couple of things
      int r_goodfrm = run_table->getRef<int>("r_goodfrm", 0);
      TS_ASSERT_EQUALS(r_goodfrm, 9229);

      int r_dur = run_table->getRef<int>("r_dur", 0);
      TS_ASSERT_EQUALS(r_dur, 462);

      double r_gd_prtn_chrg = run_table->getRef<double>("r_gd_prtn_chrg", 0);
      TS_ASSERT_DELTA(r_gd_prtn_chrg, 10.0409, 1e-4);

      std::string r_enddate = run_table->getRef<std::string>("r_enddate", 0);
      TS_ASSERT_EQUALS(r_enddate, "18-DEC-2008");
	
      //Tidy up
      Mantid::API::AnalysisDataService::Instance().remove("Raw_RPB");
    }

  }
  

private:
  // This assumes the directory structure of the repository (i.e the buildserver)
  const std::string m_filetotest;
};

#endif
