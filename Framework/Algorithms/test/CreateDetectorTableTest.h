// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CREATEDETECTORTABLETEST_H_
#define CREATEDETECTORTABLETEST_H_

#include "MantidAlgorithms/CreateDetectorTable.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CreateDetectorTableTest : public CxxTest::TestSuite {
public:
  static CreateDetectorTableTest *createSuite() {
    return new CreateDetectorTableTest();
  }

  static void destroySuite(CreateDetectorTableTest *suite) { delete suite; }

  void testName() {
    CreateDetectorTable alg;
    //TS_ASSERT_EQUALS(alg.name(), "CreateDetectorTable");
  }

  void testVersion() {
    //Mantid::Algorithms::CreateDetectorTable alg;
    //TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void testInit() {
    /*CreateDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    const auto &props = alg.getProperties();
    TS_ASSERT_EQUALS(props.size(), 4);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());

    TS_ASSERT_EQUALS(props[1]->name(), "WorkspaceIndices");
    TS_ASSERT(props[1]->isDefault());

	TS_ASSERT_EQUALS(props[2]->name(), "IncludeData");
    TS_ASSERT(props[2]->isDefault());

    TS_ASSERT_EQUALS(props[3]->name(), "DetectorTableWorkspace");
    TS_ASSERT(props[3]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<TableWorkspace> *>(props[2]));*/
  }
};

#endif // CREATEDETECTORTABLETEST_H_
