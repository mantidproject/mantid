#ifndef MANTID_DATAHANDLING_LOADNXCANSASTEST_H_
#define MANTID_DATAHANDLING_LOADNXCANSASTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadNXcanSAS.h"
#include "NXcanSASTestHelper.h"

using Mantid::DataHandling::LoadNXcanSAS;

class LoadNXcanSASTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadNXcanSASTest *createSuite() { return new LoadNXcanSASTest(); }
  static void destroySuite(LoadNXcanSASTest *suite) { delete suite; }

  void test_that_1D_workspace_with_Q_resolution_can_be_loaded() {
    // Arrange

  }

  void test_that_1D_workspace_without_Q_resolution_can_be_loaded() {

  }

  void test_that_1D_workspace_with_transmissions_can_be_loaded() {

  }

  void test_that_invalid_file_is_rejected() {

  }

  void test_that_2D_workspace_can_be_loaded() {

  }

};

#endif /* MANTID_DATAHANDLING_LOADNXCANSASTEST_H_ */
