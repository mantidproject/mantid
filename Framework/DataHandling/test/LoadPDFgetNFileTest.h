// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidDataHandling/LoadPDFgetNFile.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"

using Mantid::DataHandling::LoadPDFgetNFile;
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class LoadPDFgetNFileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadPDFgetNFileTest *createSuite() { return new LoadPDFgetNFileTest(); }
  static void destroySuite(LoadPDFgetNFileTest *suite) { delete suite; }

  /// inner functionality for running the algorithm and doing simple checks
  DataObjects::Workspace2D_sptr runLoadPDFgetNFile(const std::string &datafilename, const std::string &wksp_name,
                                                   const size_t expNumHist, const std::string &expUnits) {
    LoadPDFgetNFile loader;
    loader.initialize();
    TS_ASSERT(loader.isInitialized());

    // Set property
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Filename", datafilename));
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("OutputWorkspace", wksp_name));

    // Execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Get the output
    DataObjects::Workspace2D_sptr outws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve(wksp_name));

    // universal checks
    TS_ASSERT(outws);
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), expNumHist);
    TS_ASSERT_EQUALS(outws->getAxis(0)->unit()->unitID(), UnitFactory::Instance().create(expUnits)->unitID());

    // Return the workspace
    return outws;
  }

  /** Test to load .sq file
   */
  void test_LoadSqFile() {
    // run the algorithm and get the output
    const std::string OUTPUT_NAME("NOM_Sqa");
    auto outws = runLoadPDFgetNFile("NOM_5429.sqa", OUTPUT_NAME, 2, "MomentumTransfer");

    // compare the results
    TS_ASSERT_DELTA(outws->x(0)[2], 0.17986950, 1.0E-8);

    // cleanup
    AnalysisDataService::Instance().remove(OUTPUT_NAME);
  }

  /** Test to load .gr file
   */
  void test_LoadGrFile() {
    // run the algorithm and get the output
    const std::string OUTPUT_NAME("NOM_Gr");
    auto outws = runLoadPDFgetNFile("NOM_5429.gr", OUTPUT_NAME, 1, "AtomicDistance");

    // cleanup
    AnalysisDataService::Instance().remove(OUTPUT_NAME);
  }

  /** Test to load .bsmo file.
   * .bsmo and .braw file record Q in descending order.
   */
  void test_LoadBackgroundFile() {
    // run the algorithm and get the output
    const std::string OUTPUT_NAME("NOM_SmoothBackground");
    auto outws = runLoadPDFgetNFile("NOM_5429.bsmo", OUTPUT_NAME, 2, "MomentumTransfer");

    // cleanup
    AnalysisDataService::Instance().remove(OUTPUT_NAME);
  }
};
