// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/SpatialGrouping.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidIndexing/IndexInfo.h"

#include <filesystem>
#include <fstream>

using namespace Mantid;

class SpatialGroupingTest : public CxxTest::TestSuite {
public:
  void testMetaInfo() {
    Algorithms::SpatialGrouping alg;
    TS_ASSERT_EQUALS(alg.name(), "SpatialGrouping");
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void testInit() {
    Algorithms::SpatialGrouping alg;
    TS_ASSERT(!alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExec() {
    const int nhist(18);
    auto workspace =
        DataObjects::create<DataObjects::Workspace2D>(ComponentCreationHelper::createTestInstrumentCylindrical(2),
                                                      Indexing::IndexInfo(nhist), HistogramData::BinEdges(2));

    Algorithms::SpatialGrouping alg;
    alg.initialize();
    alg.setProperty<Mantid::API::MatrixWorkspace_sptr>("InputWorkspace", std::move(workspace));
    alg.setProperty("Filename", "test_SpatialGrouping");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    std::string file = alg.getProperty("Filename");

    const bool fileExists = std::filesystem::exists(file);

    TSM_ASSERT(file, fileExists);

    std::ifstream input;
    input.open(file.c_str());

    /* output should match:

    <?xml version="1.0" encoding="UTF-8" ?>
    <!-- XML Grouping File created by SpatialGrouping Algorithm -->
    <detector-grouping>
    <group name="group1"><detids val="1,2,3,4,5,6,7,8,9"/></group>
    <group name="group2"><detids val="10,11,12,13,14,15,16,17,18"/></group>
    </detector-grouping>

    */

    TS_ASSERT(input.is_open());

    // Testing file
    std::vector<std::string> expected{"<?xml version=\"1.0\" encoding=\"UTF-8\" "
                                      "?>",
                                      "<!-- XML Grouping File created by "
                                      "SpatialGrouping Algorithm -->",
                                      "<detector-grouping>",
                                      "<group name=\"group1\"><detids "
                                      "val=\"1,2,3,4,5,6,7,8,9\"/></group>",
                                      "<group name=\"group2\"><detids "
                                      "val=\"10,11,12,13,14,15,16,17,18\"/></"
                                      "group>",
                                      "</detector-grouping>"};

    std::vector<std::string>::iterator iter = expected.begin();

    while (input.good() && iter != expected.end()) {
      std::string received;
      getline(input, received);

      TS_ASSERT_EQUALS(received, *iter);
      ++iter;
    }

    // close file
    input.close();

    // delete file
    remove(file.c_str());
  }
};
