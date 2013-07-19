#ifndef MANTID_DATAHANDLING_MODIFYDETECTORDOTDAYFILETEST_H_
#define MANTID_DATAHANDLING_MODIFYDETECTORDOTDAYFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/ModifyDetectorDotDatFile.h"
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class ModifyDetectorDotDatFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ModifyDetectorDotDatFileTest *createSuite() { return new ModifyDetectorDotDatFileTest(); }
  static void destroySuite( ModifyDetectorDotDatFileTest *suite ) { delete suite; }


  void test_Init()
  {
    ModifyDetectorDotDatFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    ModifyDetectorDotDatFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    // Create input workspace


    // Test Properties
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputFilename", "detector_few_maps.dat") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputFilename", "detector_few_maps_result.dat") );

    //TS_ASSERT_THROWS_NOTHING( alg.setProperty("ChunkSize", 10) );
    //TS_ASSERT_THROWS_NOTHING( alg.setProperty("NumChunks", 20) );
    //TS_ASSERT_THROWS_NOTHING( alg.execute(); );
   // TS_ASSERT( alg.isExecuted() );
    
   // Poco::File file(fullFile);
   // TS_ASSERT( file.exists() );
   // if (file.exists())
  //    file.remove();
  }
  

};


#endif /* MANTID_DATAHANDLING_MODIFYDETECTORDOTDAYFILETEST_H_ */
