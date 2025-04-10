// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/CombineTableWorkspaces.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using Mantid::Algorithms::CombineTableWorkspaces;
using Mantid::API::TableRow;
using Mantid::DataObjects::TableWorkspace;
using Mantid::DataObjects::TableWorkspace_sptr;
using Mantid::Kernel::V3D;

/** Create a table workspace
 * @param types A vector of strings of the data types of the columns (either int or string for the purpose of tesing)
 * @param rowCount The number of detectors required
 * @param types A vector of strings of the names of the columns
 * @return A pointer to the table workspace
 */
template <typename T>
TableWorkspace_sptr createSingleTypeTableWorkspace(std::string &dataType, const int rowCount,
                                                   const std::vector<std::string> &names, const T &defaultVal) {
  auto table = std::make_shared<TableWorkspace>();
  for (auto &name : names) {
    table->addColumn(dataType, name);
  }
  for (int row = 0; row < rowCount; ++row) {
    TableRow newRow = table->appendRow();
    for (int col = 0; col < names.size(); col++) {
      newRow << defaultVal;
    }
  }
  return table;
}

template <typename T1, typename T2>
TableWorkspace_sptr createMultiTypeTableWorkspace(std::pair<std::string, std::string> &dataTypes, const int rowCount,
                                                  const std::pair<std::string, std::string> &names,
                                                  const T1 &defaultVal1, const T2 &defaultVal2) {
  auto table = std::make_shared<TableWorkspace>();
  table->addColumn(dataTypes.first, names.first);
  table->addColumn(dataTypes.second, names.second);
  for (int row = 0; row < rowCount; ++row) {
    TableRow newRow = table->appendRow();
    newRow << defaultVal1;
    newRow << defaultVal2;
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

template <typename T>
void test_identical_single_type_combine(std::string &dataType, const int rowCount,
                                        const std::vector<std::string> &names, const T &defaultVal) {
  TableWorkspace_sptr table1 = createSingleTypeTableWorkspace<T>(dataType, rowCount, names, defaultVal);
  TableWorkspace_sptr table2 = table1->clone();

  // Name of the output workspace.
  std::string outWSName("CombineTableWorkspacesTest_OutputWS");

  Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
  TS_ASSERT(alg->execute())

  // Retrieve the workspace from data service.
  TableWorkspace_sptr ws = getOutput(outWSName);

  // check properties of output table
  TS_ASSERT_EQUALS(ws->rowCount(), 2 * rowCount)
  TS_ASSERT_EQUALS(table1->getColumnNames(), table2->getColumnNames())
  // check the rows added contain the expected values
  TS_ASSERT_EQUALS(ws->cell<T>(2, 0), defaultVal)
  TS_ASSERT_EQUALS(ws->cell<T>(3, 1), defaultVal)

  // Remove workspace from the data service.
  AnalysisDataService::Instance().remove(outWSName);
}

template <typename T1, typename T2>
void setup_identical_mixed_type_combine(std::pair<std::string, std::string> &dataTypes, const int rowCount,
                                        const std::pair<std::string, std::string> &names, T1 &defaultVal1,
                                        T2 &defaultVal2) {

  TableWorkspace_sptr table1 =
      createMultiTypeTableWorkspace<T1, T2>(dataTypes, rowCount, names, defaultVal1, defaultVal2);
  TableWorkspace_sptr table2 = table1->clone();

  // Name of the output workspace.
  std::string outWSName("CombineTableWorkspacesTest_OutputWS");

  Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
  TS_ASSERT(alg->execute())

  // Retrieve the workspace from data service.
  TableWorkspace_sptr ws = getOutput(outWSName);

  // check properties of output table
  TS_ASSERT_EQUALS(ws->rowCount(), 2 * rowCount)
  TS_ASSERT_EQUALS(table1->getColumnNames(), table2->getColumnNames())
  // check the rows added contain the expected values
  TS_ASSERT_EQUALS(ws->cell<T1>(2, 0), defaultVal1)
  TS_ASSERT_EQUALS(ws->cell<T2>(3, 1), defaultVal2)

  // Remove workspace from the data service.
  AnalysisDataService::Instance().remove(outWSName);
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
    std::string dType = "int";
    std::vector<std::string> names = {"ThingA", "ThingB"};
    test_identical_single_type_combine<int>(dType, 2, names, 0);
  }

  void test_identical_bool_type_combine() {
    std::string dType = "bool";
    std::vector<std::string> names = {"ThingA", "ThingB"};
    test_identical_single_type_combine<Mantid::API::Boolean>(dType, 2, names, true);
  }

  void test_identical_double_type_combine() {
    std::string dType = "double";
    std::vector<std::string> names = {"ThingA", "ThingB"};
    test_identical_single_type_combine<double>(dType, 2, names, 0.0);
  }

  void test_identical_string_type_combine() {
    std::string dType = "str";
    std::vector<std::string> names = {"ThingA", "ThingB"};
    test_identical_single_type_combine<std::string>(dType, 2, names, "0");
  }

  void test_identical_float_type_combine() {
    std::string dType = "float";
    std::vector<std::string> names = {"ThingA", "ThingB"};
    test_identical_single_type_combine<float>(dType, 2, names, 0.0);
  }

  void test_identical_size_type_combine() {
    std::string dType = "size_t";
    std::vector<std::string> names = {"ThingA", "ThingB"};
    test_identical_single_type_combine<std::size_t>(dType, 2, names, 0);
  }

  void test_identical_v3d_type_combine() {
    std::string dType = "V3D";
    std::vector<std::string> names = {"ThingA", "ThingB"};
    test_identical_single_type_combine<V3D>(dType, 2, names, V3D(0.0, 0.0, 0.0));
  }

  void test_identical_mixed_type_combine() {
    const std::map<std::string, int> allowedColumnTypes = CombineTableWorkspaces::allowedTypes();
    const std::pair<std::string, std::string> names = {"ThingA", "ThingB"};

    double defaultDouble = 0.0;
    int defaultInt = 0;
    std::string defaultString = "0";
    Mantid::API::Boolean defaultBool = true;
    std::size_t defaultSize = 0;
    float defaultFloat = 0.0;
    V3D defaultV3D = V3D(0.0, 0.0, 0.0);

    std::string doubleString = "double";
    std::string intString = "int";
    std::string stringString = "str";
    std::string boolString = "bool";
    std::string sizeString = "size_t";
    std::string floatString = "float";
    std::string V3DString = "V3D";

    // doubles
    std::pair<std::string, std::string> dTypes = {doubleString, intString};
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultDouble, defaultInt);

    dTypes.second = stringString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultDouble, defaultString);

    dTypes.second = boolString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultDouble, defaultBool);

    dTypes.second = sizeString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultDouble, defaultSize);

    dTypes.second = floatString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultDouble, defaultFloat);

    dTypes.second = V3DString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultDouble, defaultV3D);

    // ints

    dTypes.first = intString;
    dTypes.second = stringString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultInt, defaultString);

    dTypes.second = boolString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultInt, defaultBool);

    dTypes.second = sizeString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultInt, defaultSize);

    dTypes.second = floatString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultInt, defaultFloat);

    dTypes.second = V3DString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultInt, defaultV3D);

    // strings
    dTypes.first = stringString;
    dTypes.second = boolString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultString, defaultBool);

    dTypes.second = sizeString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultString, defaultSize);

    dTypes.second = floatString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultString, defaultFloat);

    dTypes.second = V3DString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultString, defaultV3D);

    // bools
    dTypes.first = boolString;
    dTypes.second = sizeString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultBool, defaultSize);

    dTypes.second = floatString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultBool, defaultFloat);

    dTypes.second = V3DString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultBool, defaultV3D);

    // size_ts
    dTypes.first = sizeString;
    dTypes.second = floatString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultSize, defaultFloat);

    dTypes.second = V3DString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultSize, defaultV3D);

    // floats
    dTypes.first = floatString;
    dTypes.second = V3DString;
    setup_identical_mixed_type_combine(dTypes, 2, names, defaultFloat, defaultV3D);
  }

  // rest of the tests will just be done on on example type combination as functionality should be type independent

  void test_different_single_type_combine() {
    std::string dType = "int";
    std::vector<std::string> names = {"ThingA", "ThingB"};
    TableWorkspace_sptr table1 = createSingleTypeTableWorkspace<int>(dType, 2, names, 0);
    TableWorkspace_sptr table2 = createSingleTypeTableWorkspace<int>(dType, 3, names, 1);

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
    TS_ASSERT_EQUALS(ws->cell<int>(1, 0), 0)
    TS_ASSERT_EQUALS(ws->cell<int>(2, 1), 1)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_different_tables_with_different_types_but_same_columns_combine() {
    std::pair<std::string, std::string> dTypes = {"int", "double"};
    std::pair<std::string, std::string> colTitles = {"ThingA", "ThingB"};
    TableWorkspace_sptr table1 = createMultiTypeTableWorkspace<int, double>(dTypes, 2, colTitles, 0, 0.0);
    TableWorkspace_sptr table2 = createMultiTypeTableWorkspace<int, double>(dTypes, 3, colTitles, 1, 1.0);

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
    TS_ASSERT_EQUALS(ws->cell<int>(1, 0), 0)
    TS_ASSERT_EQUALS(ws->cell<int>(2, 0), 1)
    TS_ASSERT_EQUALS(ws->cell<double>(1, 1), 0.0)
    TS_ASSERT_EQUALS(ws->cell<double>(2, 1), 1.0)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  // failures

  void test_different_number_of_columns_throw_error() {
    std::string dType = "int";
    std::vector<std::string> colTitles1 = {"ThingA", "ThingB"};
    std::vector<std::string> colTitles2 = {"ThingA", "ThingB", "ThisOtherThing"};
    TableWorkspace_sptr table1 = createSingleTypeTableWorkspace<int>(dType, 2, colTitles1, 0);
    TableWorkspace_sptr table2 = createSingleTypeTableWorkspace<int>(dType, 2, colTitles2, 0);

    // Name of the output workspace.
    std::string outWSName("CombineTableWorkspacesTest_OutputWS");

    Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &)
  }

  void test_different_column_names_throw_error() {
    std::string dType = "int";
    std::vector<std::string> colTitles1 = {"ThingA", "ThingB"};
    std::vector<std::string> colTitles2 = {"ThingC", "ThingD"};
    TableWorkspace_sptr table1 = createSingleTypeTableWorkspace<int>(dType, 2, colTitles1, 0);
    TableWorkspace_sptr table2 = createSingleTypeTableWorkspace<int>(dType, 2, colTitles2, 0);

    // Name of the output workspace.
    std::string outWSName("CombineTableWorkspacesTest_OutputWS");

    Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &)
  }

  void test_different_types_throw_error() {
    std::string dType1 = "int";
    std::string dType2 = "double";
    std::vector<std::string> colTitles = {"ThingA", "ThingB"};
    TableWorkspace_sptr table1 = createSingleTypeTableWorkspace<int>(dType1, 2, colTitles, 0);
    TableWorkspace_sptr table2 = createSingleTypeTableWorkspace<double>(dType2, 2, colTitles, 0.0);

    // Name of the output workspace.
    std::string outWSName("CombineTableWorkspacesTest_OutputWS");

    Mantid::API::IAlgorithm_sptr alg = setupAlg(table1, table2, outWSName);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &)
  }
};
