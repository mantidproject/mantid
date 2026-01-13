// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidMDAlgorithms/SaveZODS.h"

#include <cxxtest/TestSuite.h>

#include <filesystem>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

using Mantid::coord_t;

class SaveZODSTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SaveZODS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  std::string do_test(const std::string &InputWorkspace, const std::string &Filename, bool expectSuccess = true) {
    SaveZODS alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", InputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", Filename));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    if (expectSuccess) {
      TS_ASSERT(alg.isExecuted());
    } else {
      TS_ASSERT(!alg.isExecuted());
    }
    // Return full path to file
    return alg.getPropertyValue("Filename");
  }

  void test_exec() {
    size_t numBins[3] = {10, 8, 2};
    coord_t min[3] = {0, 10, 0};
    coord_t max[3] = {10, 34, 10};
    MDHistoWorkspace_sptr ws =
        MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(3, 1.0, 2.0, numBins, min, max, "mdhisto3");
    for (size_t x = 0; x < 10; x++)
      for (size_t y = 0; y < 8; y++)
        for (size_t z = 0; z < 2; z++)
          ws->setSignalAt(ws->getLinearIndex(x, y, z), double(x + 10 * y + 100 * z));
    // Actually do the test
    std::string Filename = do_test("mdhisto3", "SaveZODS_test.h5");

    // Check the results
    TS_ASSERT(std::filesystem::exists(Filename));
    if (std::filesystem::exists(Filename))
      std::filesystem::remove(Filename);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("mdhisto3");
  }
};
