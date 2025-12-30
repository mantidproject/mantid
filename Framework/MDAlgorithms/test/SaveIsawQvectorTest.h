// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidMDAlgorithms/SaveIsawQvector.h"

#include <cxxtest/TestSuite.h>
#include <filesystem>

using Mantid::API::AnalysisDataService;
using Mantid::MDAlgorithms::SaveIsawQvector;

class SaveIsawQvectorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveIsawQvectorTest *createSuite() { return new SaveIsawQvectorTest(); }
  static void destroySuite(SaveIsawQvectorTest *suite) { delete suite; }

  void test_Init() {
    SaveIsawQvector alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string inWSName("SaveIsawQvectorTest_InputWS");
    std::string outfile = "./SaveIsawQvectorTest.bin";

    // create the test workspace
    int numEventsPer = 100;
    Mantid::DataObjects::EventWorkspace_sptr inputW =
        Mantid::DataObjects::MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer);
    AnalysisDataService::Instance().addOrReplace(inWSName, inputW);
    size_t nevents = inputW->getNumberEvents();

    // run the actual algorithm
    SaveIsawQvector alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outfile));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Get the file
    outfile = alg.getPropertyValue("Filename");
    TS_ASSERT(std::filesystem::exists(outfile));
    std::size_t bytes = std::filesystem::file_size(outfile);
    TS_ASSERT_EQUALS(bytes / (3 * 4),
                     nevents); // 3 floats (Qx,Qy,Qz) for each event

    if (std::filesystem::exists(outfile))
      std::filesystem::remove(outfile);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
  }
};
