// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADNEXUSPROCESSED2TEST_H_
#define MANTID_DATAHANDLING_LOADNEXUSPROCESSED2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadNexusProcessed2.h"

using Mantid::DataHandling::LoadNexusProcessed2;

class LoadNexusProcessed2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadNexusProcessed2Test *createSuite() {
    return new LoadNexusProcessed2Test();
  }
  static void destroySuite(LoadNexusProcessed2Test *suite) { delete suite; }

  void test_Init() {
    LoadNexusProcessed2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};

#endif /* MANTID_DATAHANDLING_LOADNEXUSPROCESSED2TEST_H_ */
