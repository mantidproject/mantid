// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/CombineTableWorkspaces.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidAlgorithms/CreateSampleWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using Mantid::Algorithms::CombineTableWorkspaces;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::API::TableRow;
using Mantid::DataObjects::TableWorkspace;
using Mantid::DataObjects::TableWorkspace_sptr;

/** Create a table workspace
 * @param types A vector of strings of the data types of the columns (either int or string for the purpose of tesing)
 * @param rowCount The number of detectors required
 * @param types A vector of strings of the names of the columns
 * @return A pointer to the table workspace
 */
TableWorkspace_sptr createTableWorkspace(const std::vector<std::string> &dataTypes, const int rowCount,
                                         const std::vector<std::string> &names, const int defaultInt = 0,
                                         const std::string &defaultString = "0", const bool defaultBool = true,
                                         const double defaultDouble = 0.0) {
  auto table = std::make_shared<TableWorkspace>();
  for (std::vector<std::string>::size_type i = 0; i < dataTypes.size(); i++) {
    if (i >= names.size()) {
      table->addColumn(dataTypes[i], "N/A");
    } else {
      table->addColumn(dataTypes[i], names[i]);
    }
  }
  for (int row = 0; row < rowCount; ++row) {
    TableRow newRow = table->appendRow();
    for (int col = 0; col < dataTypes.size(); col++) {
      std::string dType = dataTypes[col];
      if (dType == "int") {
        newRow << defaultInt;
      } else if (dType == "str") {
        newRow << defaultString;
      } else if (dType == "bool") {
        newRow << defaultBool;
      } else if (dType == "double") {
        newRow << defaultDouble;
      }
    }
  }
  return table;
}

Mantid::API::IAlgorithm_sptr setupAlg(DataObjects::TableWorkspace_sptr LHSWorkspace,
                                      DataObjects::TableWorkspace_sptr RHSWorkspace, std::string outputWS) {
  // set up algorithm
  auto alg = std::make_shared<CombineTableWorkspaces>();
  TS_ASSERT_THROWS_NOTHING(alg->initialize());
  TS_ASSERT(alg->isInitialized());
  TS_ASSERT_THROWS_NOTHING(alg->setProperty("LHSWorkspace", LHSWorkspace));
  TS_ASSERT_THROWS_NOTHING(alg->setProperty("RHSWorkspace", RHSWorkspace));
  TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", outputWS))
  return alg;
}

TableWorkspace_sptr getOutput(std::string outputWS) {
  TableWorkspace_sptr ws;
  TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(outputWS));
  TS_ASSERT(ws);
  return ws;
}

class CombineTableWorkspacesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CombineTableWorkspacesTest *createSuite() { return new CombineTableWorkspacesTest(); }
  static void destroySuite(CombineTableWorkspacesTest *suite) { delete suite; }

  void test_init() {
    CombineTableWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_identical_int_type_combine() {
    std::vector<std::string> dTypes = {"int", "int"};
    std::vector<std::string> colTitles = {"ThingA", "ThingB"};
    TableWorkspace_sptr table1 = createTableWorkspace(dTypes, 2, colTitles);
    TableWorkspace_sptr table2 = table1->clone();

    // Name of the output workspace.
    std::string outWSName("CombineTableWorkspacesTest_OutputWS");

    Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
    TS_ASSERT(alg->execute())

    // Retrieve the workspace from data service.
    TableWorkspace_sptr ws = getOutput(outWSName);

    // check properties of output table
    TS_ASSERT_EQUALS(ws->rowCount(), 4)
    TS_ASSERT_EQUALS(table1->getColumnNames(), table2->getColumnNames())
    // check the rows added contain the expected values
    TS_ASSERT_EQUALS(ws->cell<int>(2, 0), 0)
    TS_ASSERT_EQUALS(ws->cell<int>(3, 1), 0)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_identical_double_type_combine() {
    std::vector<std::string> dTypes = {"double", "double"};
    std::vector<std::string> colTitles = {"ThingA", "ThingB"};
    TableWorkspace_sptr table1 = createTableWorkspace(dTypes, 2, colTitles);
    TableWorkspace_sptr table2 = table1->clone();

    // Name of the output workspace.
    std::string outWSName("CombineTableWorkspacesTest_OutputWS");

    Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
    TS_ASSERT(alg->execute())

    // Retrieve the workspace from data service.
    TableWorkspace_sptr ws = getOutput(outWSName);

    // check properties of output table
    TS_ASSERT_EQUALS(ws->rowCount(), 4)
    TS_ASSERT_EQUALS(table1->getColumnNames(), table2->getColumnNames())
    // check the rows added contain the expected values
    TS_ASSERT_EQUALS(ws->cell<double>(2, 0), 0.0)
    TS_ASSERT_EQUALS(ws->cell<double>(3, 1), 0.0)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_identical_string_type_combine() {
    std::vector<std::string> dTypes = {"str", "str"};
    std::vector<std::string> colTitles = {"ThingA", "ThingB"};
    TableWorkspace_sptr table1 = createTableWorkspace(dTypes, 2, colTitles);
    TableWorkspace_sptr table2 = table1->clone();

    // Name of the output workspace.
    std::string outWSName("CombineTableWorkspacesTest_OutputWS");

    Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
    TS_ASSERT(alg->execute())

    // Retrieve the workspace from data service.
    TableWorkspace_sptr ws = getOutput(outWSName);

    // check properties of output table
    TS_ASSERT_EQUALS(ws->rowCount(), 4)
    TS_ASSERT_EQUALS(table1->getColumnNames(), table2->getColumnNames())
    // check the rows added contain the expected values
    TS_ASSERT_EQUALS(ws->cell<std::string>(2, 0), "0")
    TS_ASSERT_EQUALS(ws->cell<std::string>(3, 1), "0")

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_identical_bool_type_combine() {
    std::vector<std::string> dTypes = {"bool", "bool"};
    std::vector<std::string> colTitles = {"ThingA", "ThingB"};
    TableWorkspace_sptr table1 = createTableWorkspace(dTypes, 2, colTitles);
    TableWorkspace_sptr table2 = table1->clone();

    // Name of the output workspace.
    std::string outWSName("CombineTableWorkspacesTest_OutputWS");

    Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
    TS_ASSERT(alg->execute())

    // Retrieve the workspace from data service.
    TableWorkspace_sptr ws = getOutput(outWSName);

    // check properties of output table
    TS_ASSERT_EQUALS(ws->rowCount(), 4)
    TS_ASSERT_EQUALS(table1->getColumnNames(), table2->getColumnNames())
    // check the rows added contain the expected values
    TS_ASSERT_EQUALS(ws->cell<Mantid::API::Boolean>(2, 0), static_cast<Mantid::API::Boolean>(true))
    TS_ASSERT_EQUALS(ws->cell<Mantid::API::Boolean>(3, 1), static_cast<Mantid::API::Boolean>(true))

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_different_single_type_combine() {
    std::vector<std::string> dTypes = {"int", "int"};
    std::vector<std::string> colTitles = {"ThingA", "ThingB"};
    TableWorkspace_sptr table1 = createTableWorkspace(dTypes, 2, colTitles);
    TableWorkspace_sptr table2 = createTableWorkspace(dTypes, 3, colTitles, 1);

    // Name of the output workspace.
    std::string outWSName("CombineTableWorkspacesTest_OutputWS");

    Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
    TS_ASSERT(alg->execute())

    // Retrieve the workspace from data service.
    TableWorkspace_sptr ws = getOutput(outWSName);

    // check properties of output table
    TS_ASSERT_EQUALS(ws->rowCount(), 5)
    TS_ASSERT_EQUALS(table1->getColumnNames(), table2->getColumnNames())
    // check the rows added contain the expected values
    TS_ASSERT_EQUALS(ws->cell<int>(2, 0), 1)
    TS_ASSERT_EQUALS(ws->cell<int>(3, 1), 1)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_identical_tables_with_different_types_combine() {
    std::vector<std::string> dTypes = {"int", "str"};
    std::vector<std::string> colTitles = {"ThingA", "ThingB"};
    TableWorkspace_sptr table1 = createTableWorkspace(dTypes, 2, colTitles);
    TableWorkspace_sptr table2 = table1->clone();

    // Name of the output workspace.
    std::string outWSName("CombineTableWorkspacesTest_OutputWS");

    Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
    TS_ASSERT(alg->execute())

    // Retrieve the workspace from data service.
    TableWorkspace_sptr ws = getOutput(outWSName);

    // check properties of output table
    TS_ASSERT_EQUALS(ws->rowCount(), 4)
    TS_ASSERT_EQUALS(table1->getColumnNames(), table2->getColumnNames())
    // check the rows added contain the expected values
    TS_ASSERT_EQUALS(ws->cell<int>(2, 0), 0)
    TS_ASSERT_EQUALS(ws->cell<std::string>(3, 1), "0")

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};
