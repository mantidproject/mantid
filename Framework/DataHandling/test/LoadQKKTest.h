// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadQKK.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::ConfigServiceImpl;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class LoadQKKTest : public CxxTest::TestSuite {
public:
  static LoadQKKTest *createSuite() { return new LoadQKKTest(); }
  static void destroySuite(LoadQKKTest *suite) { delete suite; }

  // A sample file is in the repository
  LoadQKKTest() {}

  void test_File_Check_Confidence() {
    Mantid::DataHandling::LoadQKK loader;
    loader.initialize();
    loader.setPropertyValue("Filename",
                            "QKK0029775.nx.hdf"); // find the full path
    Mantid::Kernel::NexusDescriptor descr(loader.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(80, loader.confidence(descr));
  }

  void testInit() {
    Mantid::DataHandling::LoadQKK load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    TS_ASSERT(load.isInitialized());
  }

  void testLoad() {
    std::string wsName = "QKK0029775";
    Mantid::DataHandling::LoadQKK load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    load.setPropertyValue("Filename", "QKK0029775.nx.hdf");
    load.setPropertyValue("OutputWorkspace", wsName);
    load.execute();
    TS_ASSERT(load.isExecuted());

    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieve(wsName));
    Workspace2D_sptr data = std::dynamic_pointer_cast<Workspace2D>(ws);
    TS_ASSERT(data);
    TS_ASSERT_EQUALS(data->getNumberHistograms(), 192 * 192);
    const auto &spectrumInfo = data->spectrumInfo();
    for (size_t i = 0; i < data->getNumberHistograms(); ++i) {
      TS_ASSERT_THROWS_NOTHING(spectrumInfo.detector(i));

      const auto &x = data->x(i);
      TS_ASSERT_EQUALS(x.size(), 2);
      TS_ASSERT_DELTA(x[0], 4.9639999139, 1e-8);
      TS_ASSERT_DELTA(x[1], 5.1039999245, 1e-8);

      const auto &y = data->y(i);
      TS_ASSERT_DIFFERS(y[0], 0.0);

      const auto &e = data->e(i);
      TS_ASSERT_DIFFERS(e[0], 0.0);
    }
  }
};
