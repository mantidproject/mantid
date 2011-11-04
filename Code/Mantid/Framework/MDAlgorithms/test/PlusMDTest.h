#ifndef MANTID_MDEVENTS_PLUSMDEWTEST_H_
#define MANTID_MDEVENTS_PLUSMDEWTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDAlgorithms/PlusMD.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;

class PlusMDTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    PlusMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void do_test(bool lhs_file, bool rhs_file, int inPlace, bool deleteFile=true)
  {
    // Make two input workspaces
    MDEventWorkspace3Lean::sptr lhs = MDEventsTestHelper::makeFileBackedMDEW("PlusMDTest_lhs", lhs_file);
    MDEventWorkspace3Lean::sptr rhs = MDEventsTestHelper::makeFileBackedMDEW("PlusMDTest_rhs", rhs_file);
    std::string outWSName = "PlusMDTest_out";
    if (inPlace == 1)
      outWSName = "PlusMDTest_lhs";
    else if (inPlace == 2)
      outWSName = "PlusMDTest_rhs";
  
    PlusMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("LHSWorkspace", "PlusMDTest_lhs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("RHSWorkspace", "PlusMDTest_rhs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws); if (!ws) return;
    
    // Check the results
    if (inPlace == 1)
      { TS_ASSERT( ws == lhs); }
    else if (inPlace == 2)
      { TS_ASSERT( ws == rhs); }
    
    if ((lhs_file || rhs_file) && !((inPlace==1) && !lhs_file && rhs_file))
      { TSM_ASSERT( "If either input WS is file backed, then the output should be too.", ws->getBoxController()->isFileBacked() ); }
    TS_ASSERT_EQUALS( ws->getNPoints(), 20000);

    TSM_ASSERT("If the workspace is file-backed, then it needs updating.", ws->fileNeedsUpdating() );

    if (ws->isFileBacked())
    {

      // Run SaveMD so as to update the file in the back
      FrameworkManager::Instance().exec("SaveMD", 4,
          "InputWorkspace", outWSName.c_str(),
          "UpdateFileBackEnd", "1");

      Mantid::API::BoxController_sptr bc = ws->getBoxController();
      std::cout << bc->getDiskMRU().getFreeSpaceMap().size() << " entries in the free space map" << std::endl;
      ::NeXus::File * file = bc->getFile();
      // The file should have an entry of 20000 points too (with some error due to the free space blocks). This means the file back-end was updated
      TS_ASSERT_DELTA(file->getInfo().dims[0], 20000, 100);

      // Close the file so you can delete it. Otherwise the following test gets confused.
      if (deleteFile)
      {
        try
        {
          file->close();
          Poco::File(bc->getFilename()).remove();
        }
        catch (...)
        {
          std::cout << "Error deleting file\n";
        }
      }
    }

  }
  
  void test_mem_plus_mem()
  { do_test(false, false, 0); }

  void test_mem_plus_mem_inPlace()
  { do_test(false, false, 1); }

  void test_mem_plus_mem_inPlace_ofRHS()
  { do_test(false, false, 2); }

  void test_file_plus_mem()
  { do_test(true, false, 0); }

  void test_file_plus_mem_inPlace()
  { do_test(true, false, 1); }

  void test_mem_plus_file()
  { do_test(false, true, 0); }

  void test_mem_plus_file_inPlace()
  { do_test(false, true, 1); }

//FIXME: Test fails on Windows7 build server. http://trac.mantidproject.org/mantid/ticket/4028
//  void test_file_plus_file()
//  { do_test(true, true, 0); }
//
//  void test_file_plus_file_inPlace()
//  { do_test(true, true, 1); }
//
//  void test_file_plus_file_inPlace_ofRHS()
//  { do_test(true, true, 2); }

};


#endif /* MANTID_MDEVENTS_PLUSMDEWTEST_H_ */
