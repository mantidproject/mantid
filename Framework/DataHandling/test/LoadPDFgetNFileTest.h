#ifndef MANTID_DATAHANDLING_LOADPDFGETNFILETEST_H_
#define MANTID_DATAHANDLING_LOADPDFGETNFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadPDFgetNFile.h"
#include "MantidDataObjects/Workspace2D.h"

using Mantid::DataHandling::LoadPDFgetNFile;
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class LoadPDFgetNFileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadPDFgetNFileTest *createSuite() {
    return new LoadPDFgetNFileTest();
  }
  static void destroySuite(LoadPDFgetNFileTest *suite) { delete suite; }

  /** Test to load .sq file
   */
  void test_LoadSqFile() {
    // 1. Init
    LoadPDFgetNFile loader;
    loader.initialize();
    TS_ASSERT(loader.isInitialized());

    // 2. Set property
    std::string datafilename("NOM_5429.sqa");

    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", datafilename));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("OutputWorkspace", "NOM_Sqa"));

    // 3. Execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // 4.
    DataObjects::Workspace2D_sptr outws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::AnalysisDataService::Instance().retrieve("NOM_Sqa"));

    TS_ASSERT(outws);

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 2);

    TS_ASSERT_DELTA(outws->x(0)[2], 0.17986950, 1.0E-8);
  }

  /** Test to load .sq file
   */
  void test_LoadGrFile() {
    // 1. Init
    LoadPDFgetNFile loader;
    loader.initialize();
    TS_ASSERT(loader.isInitialized());

    // 2. Set property
    std::string datafilename("NOM_5429.gr");

    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", datafilename));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("OutputWorkspace", "NOM_Gr"));

    // 3. Execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // 4.
    DataObjects::Workspace2D_sptr outws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::AnalysisDataService::Instance().retrieve("NOM_Gr"));

    TS_ASSERT(outws);

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 1);
  }

  /** Test to load .bsmo file.
   * .bsmo and .braw file record Q in descending order.
   */
  void test_LoadBackgroundFile() {
    // 1. Init
    LoadPDFgetNFile loader;
    loader.initialize();
    TS_ASSERT(loader.isInitialized());

    // 2. Set property
    std::string datafilename("NOM_5429.bsmo");

    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", datafilename));
    TS_ASSERT_THROWS_NOTHING(
        loader.setProperty("OutputWorkspace", "NOM_SmoothBackground"));

    // 3. Execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // 4.
    DataObjects::Workspace2D_sptr outws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::AnalysisDataService::Instance().retrieve(
                "NOM_SmoothBackground"));

    TS_ASSERT(outws);

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 2);

    AnalysisDataService::Instance().remove("NOM_SmoothBackground");

    return;
  }
};

#endif /* MANTID_DATAHANDLING_LOADPDFGETNFILETEST_H_ */
