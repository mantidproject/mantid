#ifndef MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_
#define MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/SaveMask.h"
#include "MantidDataHandling/LoadMask.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"

#include "Poco/File.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveMaskTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveMaskTest *createSuite() { return new SaveMaskTest(); }

  static void destroySuite(SaveMaskTest *suite) { delete suite; }

  void test_Initialize() {
    SaveMask savealg;
    savealg.initialize();

    TS_ASSERT(savealg.isInitialized());
  }

  /*
   * (1) Load an existing masking file.  It is in 1x1
   * (2) Save masking workspace to file.
   * (3) Load the newly saved file, and compare with original masking workspace
   * Notice: the prerequisit is that LoadMask() is correct.
   */
  void test_SaveFile() {
    // 1. Init SaveDetectorMasking
    SaveMask savealg;
    savealg.initialize();

    // 2. Run LoadMask
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "POWGEN");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    TS_ASSERT_EQUALS(loadfile.execute(), true);
    DataObjects::SpecialWorkspace2D_sptr maskws =
        AnalysisDataService::Instance()
            .retrieveWS<DataObjects::SpecialWorkspace2D>("PG3Mask");

    // 3. Set property and run
    TS_ASSERT(savealg.setProperty("InputWorkspace", maskws));
    TS_ASSERT(savealg.setProperty("OutputFile", "maskcopy.xml"));

    savealg.execute();
    TS_ASSERT(savealg.isExecuted());

    // Retrieve the full path to file
    std::string file1 = savealg.getPropertyValue("OutputFile");

    // 4. Load the new XML file
    LoadMask loadfile2;
    loadfile2.initialize();

    loadfile2.setProperty("Instrument", "POWGEN");
    loadfile2.setProperty("InputFile", file1);
    loadfile2.setProperty("OutputWorkspace", "PG3MaskCopy");

    TS_ASSERT_EQUALS(loadfile2.execute(), true);
    DataObjects::SpecialWorkspace2D_sptr maskws2 =
        AnalysisDataService::Instance()
            .retrieveWS<DataObjects::SpecialWorkspace2D>("PG3MaskCopy");

    // 5. Compare
    TS_ASSERT_EQUALS(maskws->getNumberHistograms(),
                     maskws2->getNumberHistograms());
    for (size_t i = 0; i < maskws->getNumberHistograms(); i++) {
      TS_ASSERT_EQUALS(maskws->dataY(i)[0], maskws2->dataY(i)[0]);
    }

    // 6. Clean the file
    Poco::File cleanfile(file1);
    cleanfile.remove(false);
  }
};

#endif /* MANTID_DATAHANDLING_SAVEMASKINGTOFILETEST_H_ */
