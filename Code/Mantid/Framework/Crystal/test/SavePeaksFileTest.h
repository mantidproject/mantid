#ifndef MANTID_CRYSTAL_SAVEPEAKSFILETEST_H_
#define MANTID_CRYSTAL_SAVEPEAKSFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/System.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <iostream>
#include <iomanip>
#include <Poco/File.h>

#include "MantidCrystal/LoadPeaksFile.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Crystal;

#include "MantidCrystal/SavePeaksFile.h"

class SavePeaksFileTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    SavePeaksFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  //TODO: re-enable
  void xtest_exec()
  {
    LoadPeaksFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setPropertyValue("Filename", "TOPAZ_1204.peaks");
    alg.setPropertyValue("OutputWorkspace", "TOPAZ");

    TS_ASSERT( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve("TOPAZ") ) );
    TS_ASSERT(ws);
    if (!ws) return;

    SavePeaksFile alg2;
    alg2.initialize();
    alg2.setProperty("InputWorkspace", boost::dynamic_pointer_cast<Workspace>(ws) );
    alg2.setProperty("Filename", "SavePeaksFileTest.peaks");
    TS_ASSERT_THROWS_NOTHING( alg2.execute(); )
    TS_ASSERT( alg2.isExecuted() );

    std::string outFile = alg2.getPropertyValue("Filename");

    TS_ASSERT( Poco::File(outFile).exists() );

    if (Poco::File(outFile).exists())
      Poco::File(outFile).remove();

  }


};


#endif /* MANTID_CRYSTAL_SAVEPEAKSFILETEST_H_ */

