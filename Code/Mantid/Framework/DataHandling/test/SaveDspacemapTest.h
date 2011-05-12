#ifndef MANTID_DATAHANDLING_SAVEDSPACEMAPTEST_H_
#define MANTID_DATAHANDLING_SAVEDSPACEMAPTEST_H_

#include "MantidDataHandling/SaveDspacemap.h"
#include "MantidDataHandling/LoadDspacemap.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IInstrument_sptr;

class SaveDspacemapTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    SaveDspacemap alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  /**
   * @param pad :: padding parameter
   * @param expectedSize :: expected file size
   * @param removeFile :: delete the file
   * @return
   */
  std::string do_test(int pad, int expectedSize, bool removeFile)
  {
    // Name of the output workspace.
    std::string filename("./SaveDspacemapTest_Output.dat");

    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    OffsetsWorkspace_sptr offsetsWS(new OffsetsWorkspace(inst));
    offsetsWS->setValue(1,0.10);
    offsetsWS->setValue(2,0.20);
    offsetsWS->setValue(3,0.30);
  
    SaveDspacemap alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", offsetsWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DspacemapFile", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PadDetID", pad) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    filename = alg.getPropertyValue("DspacemapFile");

    TS_ASSERT( Poco::File(filename).exists() );
    
    if (Poco::File(filename).exists())
    {
      // We can only check that the size is right, more detailed checks are tricky due to weird format.
      TS_ASSERT_EQUALS( Poco::File(filename).getSize(), expectedSize);
      if (removeFile)
        Poco::File(filename).remove();
    }
    return filename;
  }


  void test_nopadding()
  {
    do_test(0, 9*8, true);
  }

  void test_padding()
  {
    do_test(1000, 1000*8, true);
  }

  void test_save_then_load()
  {
    std::string filename("./SaveDspacemapTest_Output.dat");

    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    OffsetsWorkspace_sptr offsetsWS(new OffsetsWorkspace(inst));
    offsetsWS->setValue(1,0.10);
    offsetsWS->setValue(2,0.20);
    offsetsWS->setValue(3,0.30);

    SaveDspacemap alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", offsetsWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DspacemapFile", filename) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    filename = alg.getPropertyValue("DspacemapFile");

    LoadDspacemap load;
    TS_ASSERT_THROWS_NOTHING( load.initialize() )
    TS_ASSERT( load.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( load.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(offsetsWS)) );
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("Filename", filename) );
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("FileType", "POWGEN") );
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("OutputWorkspace", "dummy") );
    TS_ASSERT_THROWS_NOTHING( load.execute(); );
    TS_ASSERT( load.isExecuted() );

    OffsetsWorkspace_sptr out;
    out = boost::dynamic_pointer_cast<OffsetsWorkspace>(AnalysisDataService::Instance().retrieve("dummy"));
    TS_ASSERT(out);
    if (!out) return;
    TS_ASSERT_DELTA( out->getValue(1), 0.10, 1e-5);
    TS_ASSERT_DELTA( out->getValue(2), 0.20, 1e-5);
    TS_ASSERT_DELTA( out->getValue(3), 0.30, 1e-5);

    if (Poco::File(filename).exists())
        Poco::File(filename).remove();
  }


};


#endif /* MANTID_DATAHANDLING_SAVEDSPACEMAPTEST_H_ */

