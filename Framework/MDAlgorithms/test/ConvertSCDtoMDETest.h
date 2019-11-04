// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CONVERTWANDSCDTOMDETEST_H_
#define MANTID_MDALGORITHMS_CONVERTWANDSCDTOMDETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidMDAlgorithms/ConvertSCDtoMDE.h"
#include "MantidMDAlgorithms/LoadMD.h"
//#include "TestHelpers/WorkspaceCreationHelpers.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
using Mantid::MDAlgorithms::ConvertSCDtoMDE;
using Mantid::MDAlgorithms::LoadMD;

class ConvertSCDtoMDETest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertSCDtoMDETest *createSuite() { return new ConvertSCDtoMDETest(); }
  static void destroySuite( ConvertSCDtoMDETest *suite ) { delete suite; }


  void test_Init()
  {
    ConvertSCDtoMDE alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Create test input if necessary
    LoadMD loader;
    //loader.setChild(true);
    loader.initialize();
    loader.setPropertyValue(
        "Filename",
        Mantid::API::FileFinder::Instance().getFullPath("MAPS_MDEW.nxs"));
    loader.setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
    loader.setProperty("FileBackEnd", false);
    loader.execute();
    auto inputWS = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IMDHistoWorkspace>("MD_EVENT_WS_ID");

    ConvertSCDtoMDE alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", "MD_EVENT_WS_ID") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "_unused_for_child") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from the algorithm. The type here will probably need to change. It should
    // be the type using in declareProperty for the "OutputWorkspace" type.
    // We can't use auto as it's an implicit conversion.
    //Mantid::API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    //TS_ASSERT(outputWS);
    //TS_FAIL("TODO: Check the results and remove this line");
  }
  
  void test_Something()
  {
    //TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_MDALGORITHMS_CONVERTWANDSCDTOMDETEST_H_ */
