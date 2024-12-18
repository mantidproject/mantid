// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/NexusTester.h"
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class NexusTesterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusTesterTest *createSuite() { return new NexusTesterTest(); }
  static void destroySuite(NexusTesterTest *suite) { delete suite; }

  void test_Init() {
    NexusTester alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    NexusTester alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("SaveFilename", "NexusTester.nxs"));
    std::string fullFile = alg.getPropertyValue("SaveFilename");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("LoadFilename", fullFile));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChunkSize", 10));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumChunks", 20));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    Poco::File file(fullFile);
    TS_ASSERT(file.exists());
    if (file.exists())
      file.remove();

    double SaveSpeed = alg.getProperty("SaveSpeed");
    double LoadSpeed = alg.getProperty("LoadSpeed");
    TS_ASSERT_LESS_THAN(0.0, SaveSpeed);
    TS_ASSERT_LESS_THAN(0.0, LoadSpeed);
  }
};
