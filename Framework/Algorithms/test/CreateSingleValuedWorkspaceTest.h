// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CREATESINGLEVALUEDWORKSPACETEST_H_
#define CREATESINGLEVALUEDWORKSPACETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/CreateSingleValuedWorkspace.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include <cxxtest/TestSuite.h>

class CreateSingleValuedWorkspaceTest : public CxxTest::TestSuite {

public:
  void testInitNoErr() {
    TS_ASSERT_THROWS_NOTHING(algNoErr.initialize());
    TS_ASSERT(algNoErr.isInitialized());
  }

  void testExecNoErr() {
    if (!algNoErr.isInitialized())
      TS_ASSERT_THROWS_NOTHING(algNoErr.initialize());

    // First with no Error
    // Running algorithm here should throw
    TS_ASSERT_THROWS(algNoErr.execute(), const std::runtime_error &);

    // Set some properties
    std::string outputSpace("NoError");
    TS_ASSERT_THROWS_NOTHING(
        algNoErr.setPropertyValue("OutputWorkspace", outputSpace));
    TS_ASSERT_THROWS_NOTHING(algNoErr.setPropertyValue("DataValue", "3.0"));

    // Run the algorithm
    TS_ASSERT_THROWS_NOTHING(algNoErr.execute());

    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws =
            Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace));

    Mantid::DataObjects::WorkspaceSingleValue_sptr single =
        boost::dynamic_pointer_cast<Mantid::DataObjects::WorkspaceSingleValue>(
            ws);

    TS_ASSERT(ws.get() != nullptr);

    // Test the data
    TS_ASSERT_DELTA(single->x(0)[0], 0.0, 1e-08);
    TS_ASSERT_DELTA(single->y(0)[0], 3.0, 1e-08);
    TS_ASSERT_DELTA(single->e(0)[0], 0.0, 1e-08);
  }

  void testInitWithErr() {

    TS_ASSERT_THROWS_NOTHING(algWithErr.initialize());
    TS_ASSERT(algWithErr.isInitialized());
  }

  void testExecWithErr() {
    if (!algWithErr.isInitialized())
      TS_ASSERT_THROWS_NOTHING(algWithErr.initialize());

    // First with no Error
    // Running algorithm here should throw
    TS_ASSERT_THROWS(algWithErr.execute(), const std::runtime_error &);

    // Set some properties
    std::string outputSpace("WithError");
    TS_ASSERT_THROWS_NOTHING(
        algWithErr.setPropertyValue("OutputWorkspace", outputSpace));
    TS_ASSERT_THROWS_NOTHING(algWithErr.setPropertyValue("DataValue", "5.0"));
    TS_ASSERT_THROWS_NOTHING(algWithErr.setPropertyValue("ErrorValue", "2.0"));

    // Run the algorithm
    TS_ASSERT_THROWS_NOTHING(algWithErr.execute());

    // Get the workspace out
    Mantid::API::Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws =
            Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace));

    Mantid::DataObjects::WorkspaceSingleValue_sptr single =
        boost::dynamic_pointer_cast<Mantid::DataObjects::WorkspaceSingleValue>(
            ws);

    TS_ASSERT(ws.get() != nullptr);

    // Test the data
    TS_ASSERT_DELTA(single->x(0)[0], 0.0, 1e-08);
    TS_ASSERT_DELTA(single->y(0)[0], 5.0, 1e-08);
    TS_ASSERT_DELTA(single->e(0)[0], 2.0, 1e-08);
  }

private:
  Mantid::Algorithms::CreateSingleValuedWorkspace algNoErr, algWithErr;
};

#endif // CREATESINGLEVALUEDWORKSPACETEST_H_
