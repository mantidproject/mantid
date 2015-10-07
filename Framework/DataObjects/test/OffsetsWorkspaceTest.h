#ifndef MANTID_DATAOBJECTS_OFFSETSWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_OFFSETSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;

class OffsetsWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_Something() {}

  void testClone() {
    auto inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);

    OffsetsWorkspace ws(inst);
    TS_ASSERT_THROWS_NOTHING(ws.clone());
  }
};

#endif /* MANTID_DATAOBJECTS_OFFSETSWORKSPACETEST_H_ */
