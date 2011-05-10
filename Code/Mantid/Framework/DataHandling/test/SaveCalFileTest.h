#ifndef MANTID_DATAHANDLING_SAVECALFILETEST_H_
#define MANTID_DATAHANDLING_SAVECALFILETEST_H_

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/SaveCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iosfwd>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class SaveCalFileTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    SaveCalFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // --- Get an instrument -----
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5, false);

    // --- Make up some data ----
    GroupingWorkspace_sptr groupWS(new GroupingWorkspace(inst));
    OffsetsWorkspace_sptr offsetsWS(new OffsetsWorkspace(inst));
    SpecialWorkspace2D_sptr maskWS(new SpecialWorkspace2D(inst));
    groupWS->setValue(1, 12);
    groupWS->setValue(2, 23);
    groupWS->setValue(3, 45);
    offsetsWS->setValue(1, 0.123);
    offsetsWS->setValue(2, 0.456);
    maskWS->maskWorkspaceIndex(0, 0.0);

    // Name of the output workspace.
    std::string outWSName("SaveCalFileTest_OutputWS");
  
    SaveCalFile alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("GroupingWorkspace", groupWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OffsetsWorkspace", offsetsWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MaskWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(maskWS)) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "SaveCalFileTest.cal") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT( Poco::File(filename).exists() );

    std::ifstream grFile(filename.c_str());
    std::string str;
    getline(grFile,str);
    getline(grFile,str);
    getline(grFile,str);
    TS_ASSERT_EQUALS(str, "        0              1      0.1230000       0      12");
    getline(grFile,str);
    TS_ASSERT_EQUALS(str, "        1              2      0.4560000       1      23");
    getline(grFile,str);
    TS_ASSERT_EQUALS(str, "        2              3      0.0000000       1      45");


    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
    
  }
  

};


#endif /* MANTID_DATAHANDLING_SAVECALFILETEST_H_ */

