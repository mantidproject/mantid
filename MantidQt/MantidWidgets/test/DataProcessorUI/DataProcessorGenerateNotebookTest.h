#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorGenerateNotebook.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorVectorString.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTableModel.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

class DataProcessorGenerateNotebookTest : public CxxTest::TestSuite {

private:
  DataProcessorWhiteList createReflectometryWhiteList() {

    // Reflectometry white list
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run(s)", "InputWorkspace");
    whitelist.addElement("Angle", "ThetaIn");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun");
    whitelist.addElement("Q min", "MomentumTransferMinimum");
    whitelist.addElement("Q max", "MomentumTransferMaximum");
    whitelist.addElement("dQ/Q", "MomentumTransferStep");
    whitelist.addElement("Scale", "ScaleFactor");
    whitelist.addElement("Group", "Group");
    whitelist.addElement("Options", "Options");
    return whitelist;
  }
  std::map<std::string, DataProcessorPreprocessingAlgorithm>
  createReflectometryPreprocessMap(const std::string &plusPrefix = "") {

    // Reflectometry pre-process map
    return std::map<std::string, DataProcessorPreprocessingAlgorithm>{
        {"Run(s)", DataProcessorPreprocessingAlgorithm(
                       "Plus", plusPrefix, std::set<std::string>())},
        {"Transmission Run(s)",
         DataProcessorPreprocessingAlgorithm(
             "CreateTransmissionWorkspaceAuto", "TRANS_",
             std::set<std::string>{"FirstTransmissionRun",
                                   "SecondTransmissionRun", "OutputWorkspace"},
             false)}};
  }

  DataProcessorProcessingAlgorithm createReflectometryProcessor() {

    return DataProcessorProcessingAlgorithm(
        "ReflectometryReductionOneAuto",
        std::vector<std::string>{"IvsQ_", "IvsLam_"},
        std::set<std::string>{"ThetaIn", "ThetaOut", "InputWorkspace",
                              "OutputWorkspace", "OutputWorkspaceWavelength",
                              "FirstTransmissionRun", "SecondTransmissionRun"});
  }

  ITableWorkspace_sptr
  createWorkspace(const std::string &wsName,
                  const DataProcessorWhiteList &whitelist) {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    const int ncols = static_cast<int>(whitelist.size());

    for (int col = 0; col < ncols - 2; col++) {
      auto column = ws->addColumn("str", whitelist.colNameFromColIndex(col));
      column->setPlotType(0);
    }
    auto colGroup =
        ws->addColumn("int", whitelist.colNameFromColIndex(ncols - 2));
    auto colOptions =
        ws->addColumn("str", whitelist.colNameFromColIndex(ncols - 1));
    colGroup->setPlotType(0);
    colOptions->setPlotType(0);

    if (wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return ws;
  }

  ITableWorkspace_sptr
  createPrefilledWorkspace(const std::string &wsName,
                           const DataProcessorWhiteList &whitelist) {
    auto ws = createWorkspace(wsName, whitelist);
    TableRow row = ws->appendRow();
    row << "12345"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1" << 0 << "";
    row = ws->appendRow();
    row << "12346"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1" << 0 << "";
    row = ws->appendRow();
    row << "24681"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1" << 1 << "";
    row = ws->appendRow();
    row << "24682"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1" << 1 << "";
    return ws;
  }

  void createModel(const std::string &wsName) {
    DataProcessorWhiteList whitelist = createReflectometryWhiteList();
    ITableWorkspace_sptr prefilled_ws =
        createPrefilledWorkspace(wsName, whitelist);
    m_model.reset(new QDataProcessorTableModel(prefilled_ws, whitelist));
  }

  std::string m_wsName;
  std::string m_instrument;
  QDataProcessorTableModel_sptr m_model;
  std::set<int> m_rows;
  std::map<int, std::set<int>> m_groups;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorGenerateNotebookTest *createSuite() {
    return new DataProcessorGenerateNotebookTest();
  }
  static void destroySuite(DataProcessorGenerateNotebookTest *suite) {
    delete suite;
  }

  DataProcessorGenerateNotebookTest() { FrameworkManager::Instance(); }

  // Create a notebook to test
  void setUp() override {
    m_wsName = "TESTWORKSPACE";
    m_instrument = "INSTRUMENT";

    createModel(m_wsName);

    // Populate rows with every index in the model
    for (int idx = 0; idx < m_model->rowCount(); ++idx)
      m_rows.insert(idx);

    const int colGroup = static_cast<int>(m_model->columnCount() - 2);

    // Map group numbers to the set of rows in that group we want to process
    for (auto it = m_rows.begin(); it != m_rows.end(); ++it)
      m_groups[m_model->data(m_model->index(*it, colGroup)).toInt()].insert(
          *it);
  }

  void testGenerateNotebook() {

    auto notebook = Mantid::Kernel::make_unique<DataProcessorGenerateNotebook>(
        m_wsName, m_model, m_instrument, createReflectometryWhiteList(),
        std::map<std::string, DataProcessorPreprocessingAlgorithm>(),
        createReflectometryProcessor(), DataProcessorPostprocessingAlgorithm(),
        std::map<std::string, std::string>(), "", "");

    std::string generatedNotebook =
        notebook->generateNotebook(m_groups, m_rows);

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, generatedNotebook, boost::is_any_of("\n"));
    const std::string result[] = {
        "{", "   \"metadata\" : {", "      \"name\" : \"Mantid Notebook\"",
        "   },", "   \"nbformat\" : 3,", "   \"nbformat_minor\" : 0,",
        "   \"worksheets\" : [", "      {", "         \"cells\" : [",
        "            {", "               \"cell_type\" : \"markdown\",",
    };

    // Check that the first 10 lines are output as expected
    for (int i = 0; i < 11; ++i) {
      TS_ASSERT_EQUALS(notebookLines[i], result[i])
    }
  }

  void testPlot1DString() {
    std::vector<std::string> ws_names;
    ws_names.emplace_back("workspace1");
    ws_names.emplace_back("workspace2");

    std::string output = plot1DString(ws_names);

    const std::string result =
        "fig = plots([workspace1, workspace2], "
        "title=['workspace1', 'workspace2'], legendLocation=[1, 1, 4])\n";

    TS_ASSERT_EQUALS(output, result)
  }

  void testTableString() {
    std::string output =
        tableString(m_model, createReflectometryWhiteList(), m_rows);

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    const std::string result[] = {
        "Run(s) | Angle | Transmission Run(s) | Q min | Q max | dQ/Q | Scale | "
        "Group | Options",
        "--- | --- | --- | --- | --- | --- | --- | "
        "--- | ---",
        "12345 | 0.5 |  | 0.1 | 1.6 | 0.04 | 1 | 0 | ",
        "12346 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 | 0 | ",
        "24681 | 0.5 |  | 0.1 | 1.6 | 0.04 | 1 | 1 | ",
        "24682 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 | 1 | ", ""};

    int i = 0;
    for (auto it = notebookLines.begin(); it != notebookLines.end();
         ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }
  }

  void testVectorString() {
    std::vector<std::string> stringVector;
    stringVector.emplace_back("A");
    stringVector.emplace_back("B");
    stringVector.emplace_back("C");

    const std::string stringOutput = vectorString(stringVector);

    std::vector<int> intVector;
    intVector.push_back(1);
    intVector.push_back(2);
    intVector.push_back(3);

    const std::string intOutput = vectorString(intVector);

    // Test string list output is correct for vector of strings and vector of
    // ints
    TS_ASSERT_EQUALS(stringOutput, "A, B, C")
    TS_ASSERT_EQUALS(intOutput, "1, 2, 3")
  }

  void testTitleString() {
    // Test with workspace name
    std::string output = titleString("TEST_WORKSPACE");

    const std::string result[] = {
        "Processed data from workspace: TEST_WORKSPACE", "---------------", ""};

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    int i = 0;
    for (auto it = notebookLines.begin(); it != notebookLines.end();
         ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    // Test without workspace name
    std::string outputEmptyStr = titleString("");

    const std::string resultEmptyStr[] = {"Processed data", "---------------",
                                          ""};

    std::vector<std::string> notebookLinesEmptyStr;
    boost::split(notebookLinesEmptyStr, outputEmptyStr, boost::is_any_of("\n"));

    i = 0;
    for (auto it = notebookLinesEmptyStr.begin();
         it != notebookLinesEmptyStr.end(); ++it, ++i) {
      TS_ASSERT_EQUALS(*it, resultEmptyStr[i])
    }
  }

  void testPostprocessGroupString() {

    std::string userOptions = "Params = '0.1, -0.04, 2.9', StartOverlaps = "
                              "'1.4, 0.1, 1.4', EndOverlaps = '1.6, 2.9, 1.6'";

    boost::tuple<std::string, std::string> output = postprocessGroupString(
        m_rows, m_model, createReflectometryWhiteList(),
        createReflectometryPreprocessMap(), createReflectometryProcessor(),
        DataProcessorPostprocessingAlgorithm(), userOptions);

    const std::string result[] = {
        "#Post-process workspaces",
        "IvsQ_12345_12346_24681_24682, _ = Stitch1DMany(InputWorkspaces = "
        "'IvsQ_12345, IvsQ_12346, IvsQ_24681,"
        " IvsQ_24682', Params = '0.1, -0.04, 2.9', StartOverlaps = '1.4, 0.1, "
        "1.4', EndOverlaps = '1.6, 2.9, 1.6')",
        ""};

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, boost::get<0>(output), boost::is_any_of("\n"));

    int i = 0;
    for (auto it = notebookLines.begin(); it != notebookLines.end();
         ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }
  }

  void testPlotsString() {
    std::vector<std::string> unprocessed_ws;
    unprocessed_ws.emplace_back("TEST_WS1_1, TEST_WS1_2");
    unprocessed_ws.emplace_back("TEST_WS2_1, TEST_WS2_2");

    std::vector<std::string> postprocessed_ws;
    postprocessed_ws.emplace_back("TEST_WS3");
    postprocessed_ws.emplace_back("TEST_WS4");

    std::string output = plotsString(
        unprocessed_ws, boost::algorithm::join(postprocessed_ws, "_"),
        createReflectometryProcessor());

    const std::string result[] = {
        "#Group workspaces to be plotted on same axes",
        "IvsQ_groupWS = GroupWorkspaces(InputWorkspaces = 'TEST_WS1_1, "
        "TEST_WS2_1')",
        "IvsLam_groupWS = GroupWorkspaces(InputWorkspaces = 'TEST_WS1_2, "
        "TEST_WS2_2')",
        "#Plot workspaces", "fig = plots([IvsQ_groupWS, IvsLam_groupWS, "
                            "TEST_WS3_TEST_WS4], title=['IvsQ_groupWS', "
                            "'IvsLam_groupWS', 'TEST_WS3_TEST_WS4'], "
                            "legendLocation=[1, 1, 4])",
        ""};

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    int i = 0;
    for (auto it = notebookLines.begin(); it != notebookLines.end();
         ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }
  }

  void testReduceRowString() {

    std::map<std::string, std::string> userPreProcessingOptions = {
        {"Run(s)", ""}, {"Transmission Run(s)", ""}};

    boost::tuple<std::string, std::string> output = reduceRowString(
        1, m_instrument, m_model, createReflectometryWhiteList(),
        createReflectometryPreprocessMap("TOF_"),
        createReflectometryProcessor(), userPreProcessingOptions, "");

    const std::string result[] = {
        "TOF_12346 = Load(Filename = 'INSTRUMENT12346')",
        "IvsQ_TOF_12346, IvsLam_TOF_12346, _ = "
        "ReflectometryReductionOneAuto(InputWorkspace = 'TOF_12346', ThetaIn = "
        "1.5, MomentumTransferMinimum = 1.4, MomentumTransferMaximum = 2.9, "
        "MomentumTransferStep = 0.04, ScaleFactor = 1)",
        ""};

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, boost::get<0>(output), boost::is_any_of("\n"));

    int i = 0;
    for (auto it = notebookLines.begin(); it != notebookLines.end();
         ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }
  }

  void testPlusString() {

    auto reflectometryPreprocessMap = createReflectometryPreprocessMap();

    std::string output = plusString("INPUT_WS", "OUTPUT_WS",
                                    reflectometryPreprocessMap["Run(s)"], "");
    const std::string result = "OUTPUT_WS = Plus(LHSWorkspace = 'OUTPUT_WS', "
                               "RHSWorkspace = 'INPUT_WS')\n";
    TS_ASSERT_EQUALS(output, result)
  }

  void testLoadRunString() {
    boost::tuple<std::string, std::string> output =
        loadRunString("12345", m_instrument, "TOF_");
    const std::string result =
        "TOF_12345 = Load(Filename = 'INSTRUMENT12345')\n";
    TS_ASSERT_EQUALS(boost::get<0>(output), result)
  }

  void testVectorParamString() {
    std::vector<std::string> stringVector;
    stringVector.emplace_back("A");
    stringVector.emplace_back("B");
    stringVector.emplace_back("C");

    const std::string stringOutput =
        vectorParamString("PARAM_NAME", stringVector);

    TS_ASSERT_EQUALS(stringOutput, "PARAM_NAME = 'A, B, C'")
  }
};

#endif // MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H
