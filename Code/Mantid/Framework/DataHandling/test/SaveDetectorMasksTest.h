#ifndef MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_
#define MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/SaveDetectorMasks.h"
#include "MantidDataHandling/LoadMaskingFile.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"

#include "Poco/File.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveDetectorMasksTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveDetectorMasksTest *createSuite() { return new SaveDetectorMasksTest(); }

  static void destroySuite( SaveDetectorMasksTest *suite ) { delete suite; }

  void test_Initialize()
  {
    SaveDetectorMasks savealg;
    savealg.initialize();

    TS_ASSERT(savealg.isInitialized());
  }


  void test_SaveFile()
  {
    // 1. Init SaveDetectorMasking
    SaveDetectorMasks savealg;
    savealg.initialize();

    // 2. Run LoadMaskingFile
    LoadMaskingFile loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "POWGEN");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    TS_ASSERT_EQUALS(loadfile.execute(),true);
    DataObjects::SpecialWorkspace2D_sptr maskws =
          boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve("PG3Mask"));

    // 3. Set property and run
    TS_ASSERT(savealg.setProperty("InputWorkspace", maskws));
    TS_ASSERT(savealg.setProperty("OutputFile", "maskcopy.xml"));

    savealg.execute();
    TS_ASSERT(savealg.isExecuted());

    // 4. Load the new XML file
    LoadMaskingFile loadfile2;
    loadfile2.initialize();

    loadfile2.setProperty("Instrument", "POWGEN");
    loadfile2.setProperty("InputFile", "maskcopy.xml");
    loadfile2.setProperty("OutputWorkspace", "PG3MaskCopy");

    TS_ASSERT_EQUALS(loadfile2.execute(),true);
    DataObjects::SpecialWorkspace2D_sptr maskws2 =
          boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(AnalysisDataService::Instance().retrieve("PG3MaskCopy"));

    // 5. Compare
    TS_ASSERT_EQUALS(maskws->getNumberHistograms(), maskws2->getNumberHistograms());
    for (size_t i = 0; i < maskws->getNumberHistograms(); i ++){
      TS_ASSERT_EQUALS(maskws->dataY(i)[0], maskws2->dataY(i)[0]);
    }

    // 6. Clean the file
    Poco::File cleanfile("maskcopy.xml");
    cleanfile.remove(false);

  }


};


#endif /* MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_ */
