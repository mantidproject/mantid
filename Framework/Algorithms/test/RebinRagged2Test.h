// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/RebinRagged2.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::RebinRagged;

class RebinRagged2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinRagged2Test *createSuite() { return new RebinRagged2Test(); }
  static void destroySuite(RebinRagged2Test *suite) { delete suite; }

  void test_Init() {
    RebinRagged alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_event_workspace() {
    std::vector<double> xmins(200, 2600.);
    xmins[11] = 3000.;
    std::vector<double> xmaxs(200, 6200.);
    xmaxs[12] = 5000.;
    std::vector<double> deltas(200, 400.);
    deltas[13] = 600.;

    CreateSampleWorkspace createSampleWorkspaceAlgo;
    createSampleWorkspaceAlgo.setChild(true);
    createSampleWorkspaceAlgo.initialize();
    createSampleWorkspaceAlgo.setPropertyValue("WorkspaceType", "Event");
    createSampleWorkspaceAlgo.setPropertyValue("OutputWorkspace", "_unused_for_child");
    createSampleWorkspaceAlgo.execute();
    MatrixWorkspace_sptr ws = createSampleWorkspaceAlgo.getProperty("OutputWorkspace");

    RebinRagged alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMin", xmins));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XMax", xmaxs));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Delta", deltas));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 200);

    for (size_t i = 0; i < result->getNumberHistograms(); i++) {
      const auto X = result->readX(i);
      if (i == 11)
        TS_ASSERT_EQUALS(X.size(), 9)
      else if (i == 12 || i == 13)
        TS_ASSERT_EQUALS(X.size(), 7)
      else
        TS_ASSERT_EQUALS(X.size(), 10)

      const auto Y = result->readY(i);
      if (i == 13)
        for (const auto y : Y)
          TS_ASSERT_EQUALS(y, 21)
      else
        for (const auto y : Y)
          TS_ASSERT_EQUALS(y, 14)
    }
  }
};
