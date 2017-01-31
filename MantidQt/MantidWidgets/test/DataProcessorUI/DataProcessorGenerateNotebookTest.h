#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorGenerateNotebook.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorVectorString.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace testing;

class DataProcessorGenerateNotebookTest : public CxxTest::TestSuite {

private:
  // Creates a map with pre-processing instruction for reflectometry
  std::map<std::string, DataProcessorPreprocessingAlgorithm>
  reflPreprocessMap(const std::string &plusPrefix = "") {

    // Reflectometry pre-process map
    return std::map<std::string, DataProcessorPreprocessingAlgorithm>{
        {"Run(s)", DataProcessorPreprocessingAlgorithm(
                       "Plus", plusPrefix, std::set<std::string>())},
        {"Transmission Run(s)",
         DataProcessorPreprocessingAlgorithm(
             "CreateTransmissionWorkspaceAuto", "TRANS_",
             std::set<std::string>{"FirstTransmissionRun",
                                   "SecondTransmissionRun",
                                   "OutputWorkspace"})}};
  }

  // Creates a reflectometry processing algorithm
  DataProcessorProcessingAlgorithm reflProcessor() {

    return DataProcessorProcessingAlgorithm(
        "ReflectometryReductionOneAuto",
        std::vector<std::string>{"IvsQ_binned_", "IvsQ_", "IvsLam_"},
        std::set<std::string>{"ThetaIn", "ThetaOut", "InputWorkspace",
                              "OutputWorkspace", "OutputWorkspaceWavelength",
                              "FirstTransmissionRun", "SecondTransmissionRun"});
  }

  DataProcessorPostprocessingAlgorithm reflPostprocessor() {
    return DataProcessorPostprocessingAlgorithm(
        "Stitch1DMany", "IvsQ_",
        std::set<std::string>{"InputWorkspaces", "OutputWorkspace"});
  }

  // Creates a reflectometry whitelist
  DataProcessorWhiteList reflWhitelist() {

    // Reflectometry white list
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run(s)", "InputWorkspace", "", true, "TOF_");
    whitelist.addElement("Angle", "ThetaIn", "");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun", "",
                         true, "TRANS_");
    whitelist.addElement("Q min", "MomentumTransferMin", "");
    whitelist.addElement("Q max", "MomentumTransferMax", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    return whitelist;
  }

  // Creates reflectometry data
  TreeData reflData() {

    TreeData treeData;
    RowData rowData;

    rowData = {"12345", "0.5", "", "0.1", "1.6", "0.04", "1", ""};
    treeData[0][0] = rowData;
    rowData = {"12346", "1.5", "", "1.4", "2.9", "0.04", "1", ""};
    treeData[0][1] = rowData;
    rowData = {"24681", "0.5", "", "0.1", "1.6", "0.04", "1", ""};
    treeData[1][0] = rowData;
    rowData = {"24682", "1.5", "", "1.4", "2.9", "0.04", "1", ""};
    treeData[1][1] = rowData;

    return treeData;
  }

  std::string m_wsName;
  std::string m_instrument;

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
  }

  void testGenerateNotebookFirstLines() {

    auto notebook = Mantid::Kernel::make_unique<DataProcessorGenerateNotebook>(
        m_wsName, m_instrument, reflWhitelist(),
        std::map<std::string, DataProcessorPreprocessingAlgorithm>(),
        reflProcessor(), reflPostprocessor(),
        std::map<std::string, std::string>(), "", "");

    std::string generatedNotebook = notebook->generateNotebook(TreeData());

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

  void testTableStringWrongData() {
    // Whitelist and data incompatible

    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run", "Run", "");
    whitelist.addElement("Angle", "Angle", "");

    TreeData treeData = reflData();

    TS_ASSERT_THROWS_ANYTHING(tableString(treeData, whitelist));
  }

  void testTableStringOneRow() {

    // Create some tree data
    RowData rowData = {"24682", "1.5", "", "1.4", "2.9", "0.04", "1", ""};
    TreeData treeData = {{1, {{0, rowData}}}};

    std::string output = tableString(treeData, reflWhitelist());

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    const std::string result[] = {
        "Group | Run(s) | Angle | Transmission Run(s) | Q min | Q max | dQ/Q | "
        "Scale | Options",
        "--- | --- | --- | --- | --- | --- | --- | "
        "---",
        "1 | 24682 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 | ", ""};

    int i = 0;
    for (auto it = notebookLines.begin(); it != notebookLines.end();
         ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }
  }

  void testTableStringAllRows() {

    std::string output = tableString(reflData(), reflWhitelist());

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    const std::string result[] = {
        "Group | Run(s) | Angle | Transmission Run(s) | Q min | Q max | dQ/Q | "
        "Scale | Options",
        "--- | --- | --- | --- | --- | --- | --- | "
        "---",
        "0 | 12345 | 0.5 |  | 0.1 | 1.6 | 0.04 | 1 | ",
        "0 | 12346 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 | ",
        "1 | 24681 | 0.5 |  | 0.1 | 1.6 | 0.04 | 1 | ",
        "1 | 24682 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 | ", ""};

    int i = 0;
    for (const auto &line : notebookLines) {
      TS_ASSERT_EQUALS(line, result[i++])
    }
  }

  void testLoadRunString() {
    boost::tuple<std::string, std::string> output =
        loadRunString("12345", m_instrument, "TOF_");
    const std::string result =
        "TOF_12345 = Load(Filename = 'INSTRUMENT12345')\n";
    TS_ASSERT_EQUALS(boost::get<0>(output), result)
  }

  void testPlusString() {

    auto reflectometryPreprocessMap = reflPreprocessMap();

    std::string output = plusString("INPUT_WS", "OUTPUT_WS",
                                    reflectometryPreprocessMap["Run(s)"], "");
    const std::string result = "OUTPUT_WS = Plus(LHSWorkspace = 'OUTPUT_WS', "
                               "RHSWorkspace = 'INPUT_WS')\n";
    TS_ASSERT_EQUALS(output, result)
  }

  void testPlusStringWithOptions() {

    auto preprocessMap = reflPreprocessMap();
    auto transProcessor = preprocessMap["Transmission Run(s)"];
    std::string output = plusString("INPUT_WS", "OUTPUT_WS", transProcessor,
                                    "WavelengthMin = 0.5, WavelengthMax = 5.0");
    std::string result =
        "OUTPUT_WS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun "
        "= 'OUTPUT_WS', SecondTransmissionRun = 'INPUT_WS', WavelengthMin = "
        "0.5, WavelengthMax = 5.0)\n";
    TS_ASSERT_EQUALS(output, result)
  }

  void testLoadWorkspaceStringOneRun() {

    auto processor = reflPreprocessMap()["Transmission Run(s)"];
    auto output = loadWorkspaceString("RUN", "INST_", processor, "");
    TS_ASSERT_EQUALS(boost::get<1>(output), "TRANS_RUN");
    TS_ASSERT_EQUALS(boost::get<0>(output),
                     "TRANS_RUN = Load(Filename = 'INST_RUN')\n");
  }

  void testLoadWorkspaceStringThreeRunsWithOptions() {

    DataProcessorPreprocessingAlgorithm preprocessor("WeightedMean");
    auto output = loadWorkspaceString("RUN1+RUN2,RUN3", "INST_", preprocessor,
                                      "Property1 = 1, Property2 = 2");
    std::vector<std::string> outputLines;
    boost::split(outputLines, boost::get<0>(output), boost::is_any_of("\n"));

    // The python code that does the loading
    const std::string result[] = {
        "RUN1 = Load(Filename = 'INST_RUN1')", "RUN1_RUN2_RUN3 = RUN1",
        "RUN2 = Load(Filename = 'INST_RUN2')",
        "RUN1_RUN2_RUN3 = WeightedMean(InputWorkspace1 = 'RUN1_RUN2_RUN3', "
        "InputWorkspace2 = 'RUN2', Property1 = 1, Property2 = 2)",
        "RUN3 = Load(Filename = 'INST_RUN3')",
        "RUN1_RUN2_RUN3 = WeightedMean(InputWorkspace1 = 'RUN1_RUN2_RUN3', "
        "InputWorkspace2 = 'RUN3', Property1 = 1, Property2 = 2)"};
    for (int i = 0; i < 6; i++)
      TS_ASSERT_EQUALS(outputLines[i], result[i]);

    // The loaded workspace
    TS_ASSERT_EQUALS(boost::get<1>(output), "RUN1_RUN2_RUN3");
  }

  void testReduceRowStringWrongData() {
    // Whitelist and data differ in size

    RowData rowData = {"12345", "1.5"};

    TS_ASSERT_THROWS_ANYTHING(reduceRowString(
        rowData, m_instrument, reflWhitelist(), reflPreprocessMap("TOF_"),
        reflProcessor(), std::map<std::string, std::string>(), ""));
  }

  void testReduceRowString() {
    // Reduce a single row, no pre-processing is needed because there's
    // only one run in the 'Run(s)' column and no transmission runs

    std::map<std::string, std::string> userPreProcessingOptions = {
        {"Run(s)", ""}, {"Transmission Run(s)", ""}};

    const RowData data = {"12346", "1.5", "", "1.4", "2.9", "0.04", "1", ""};

    boost::tuple<std::string, std::string> output = reduceRowString(
        data, m_instrument, reflWhitelist(), reflPreprocessMap("TOF_"),
        reflProcessor(), userPreProcessingOptions, "");

    const std::string result[] = {
        "TOF_12346 = Load(Filename = 'INSTRUMENT12346')",
        "IvsQ_binned_TOF_12346, IvsQ_TOF_12346, IvsLam_TOF_12346 = "
        "ReflectometryReductionOneAuto(InputWorkspace = 'TOF_12346', ThetaIn = "
        "1.5, MomentumTransferMin = 1.4, MomentumTransferMax = 2.9, "
        "MomentumTransferStep = 0.04, ScaleFactor = 1)",
        ""};

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, boost::get<0>(output), boost::is_any_of("\n"));

    int i = 0;
    for (const auto &line : notebookLines) {
      TS_ASSERT_EQUALS(line, result[i++]);
    }
  }

  void testReduceRowStringWithPreprocessing() {
    // Reduce a single row, one column need pre-processing

    // Create a whitelist
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run", "InputWorkspace", "", true);
    whitelist.addElement("Angle", "ThetaIn", "", true, "angle_");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun", "");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");

    // Create a pre-process map
    std::map<std::string, DataProcessorPreprocessingAlgorithm> preprocessMap = {
        {"Run", DataProcessorPreprocessingAlgorithm("Plus", "RUN_",
                                                    std::set<std::string>())}};
    // Specify some pre-processing options
    std::map<std::string, std::string> userPreProcessingOptions = {
        {"Run", "Property=prop"}};

    // Create some data
    const RowData data = {"1000+1001", "0.5", "", "", "", "", "", ""};

    boost::tuple<std::string, std::string> output =
        reduceRowString(data, "INST", whitelist, preprocessMap, reflProcessor(),
                        userPreProcessingOptions, "");

    const std::string result[] = {
        "RUN_1000 = Load(Filename = 'INST1000')", "RUN_1000_1001 = RUN_1000",
        "RUN_1001 = Load(Filename = 'INST1001')",
        "RUN_1000_1001 = Plus(LHSWorkspace = 'RUN_1000_1001', RHSWorkspace = "
        "'RUN_1001', Property=prop)",
        "IvsQ_binned_1000_1001_angle_0.5, IvsQ_1000_1001_angle_0.5, "
        "IvsLam_1000_1001_angle_0.5 = "
        "ReflectometryReductionOneAuto(InputWorkspace = 'RUN_1000_1001', "
        "ThetaIn = 0.5)",
        ""};

    // Check the names of the reduced workspaces
    TS_ASSERT_EQUALS(boost::get<1>(output), "IvsQ_binned_1000_1001_angle_0.5, "
                                            "IvsQ_1000_1001_angle_0.5, "
                                            "IvsLam_1000_1001_angle_0.5");

    // Check the python code
    std::vector<std::string> notebookLines;
    boost::split(notebookLines, boost::get<0>(output), boost::is_any_of("\n"));
    int i = 0;
    for (const auto &line : notebookLines) {
      TS_ASSERT_EQUALS(line, result[i++])
    }
  }

  void testReduceRowStringNoPreProcessing() {
    // Reduce a run without pre-processing algorithm specified (i.e. empty
    // pre-process map)

    std::map<std::string, DataProcessorPreprocessingAlgorithm>
        emptyPreProcessMap;
    std::map<std::string, std::string> emptyPreProcessingOptions;

    const RowData data = {"12346", "1.5", "", "1.4", "2.9", "0.04", "1", ""};

    boost::tuple<std::string, std::string> output =
        reduceRowString(data, m_instrument, reflWhitelist(), emptyPreProcessMap,
                        reflProcessor(), emptyPreProcessingOptions, "");

    const std::string result[] = {
        "IvsQ_binned_TOF_12346, IvsQ_TOF_12346, IvsLam_TOF_12346 = "
        "ReflectometryReductionOneAuto(InputWorkspace = 12346, ThetaIn = 1.5, "
        "MomentumTransferMin = 1.4, MomentumTransferMax = 2.9, "
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

  void testReducedWorkspaceNameWrong() {
    // Whitelist and data differ in size

    // Create a whitelist
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run", "", "", true, "run_");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Trans", "", "", false, "");

    // Create some data
    const RowData data = {"1000,1001", "0.5",  "2000,2001", "1.4",
                          "2.9",       "0.04", "1",         ""};

    TS_ASSERT_THROWS_ANYTHING(
        getReducedWorkspaceName(data, whitelist, "IvsQ_"));
  }

  void testReducedWorkspaceNameOnlyRun() {

    // Create a whitelist
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run", "", "", true, "run_");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Trans", "", "", false, "");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");

    // Create some data
    const RowData data = {"1000,1001", "0.5",  "2000,2001", "1.4",
                          "2.9",       "0.04", "1",         ""};

    std::string name = getReducedWorkspaceName(data, whitelist, "IvsQ_");
    TS_ASSERT_EQUALS(name, "IvsQ_run_1000_1001")
  }

  void testReducedWorkspaceNameRunAndTrans() {

    // Create a whitelist
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run", "", "", true, "run_");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Trans", "", "", true, "trans_");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");

    // Create some data
    const RowData data = {"1000,1001", "0.5",  "2000,2001", "1.4",
                          "2.9",       "0.04", "1",         ""};

    std::string name = getReducedWorkspaceName(data, whitelist, "Prefix_");
    TS_ASSERT_EQUALS(name, "Prefix_run_1000_1001_trans_2000_2001")
  }

  void testReducedWorkspaceNameTransNoPrefix() {

    // Create a whitelist
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run", "", "", false, "");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Trans", "", "", true, "");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");

    const RowData data = {"1000,1001", "0.5",  "2000+2001", "1.4",
                          "2.9",       "0.04", "1",         ""};

    std::string name = getReducedWorkspaceName(data, whitelist, "Prefix_");
    TS_ASSERT_EQUALS(name, "Prefix_2000_2001")
  }

  void testPostprocessGroupString() {
    std::string userOptions = "Params = '0.1, -0.04, 2.9', StartOverlaps = "
                              "'1.4, 0.1, 1.4', EndOverlaps = '1.6, 2.9, 1.6'";

    RowData rowData0 = {"12345", "", "", "", "", "", "", ""};
    RowData rowData1 = {"12346", "", "", "", "", "", "", ""};
    GroupData groupData = {{0, rowData0}, {1, rowData1}};

    boost::tuple<std::string, std::string> output =
        postprocessGroupString(groupData, reflWhitelist(), reflProcessor(),
                               reflPostprocessor(), userOptions);

    std::vector<std::string> result = {
        "#Post-process workspaces",
        "IvsQ_TOF_12345_TOF_12346, _ = "
        "Stitch1DMany(InputWorkspaces = "
        "'IvsQ_binned_TOF_12345, IvsQ_binned_TOF_12346', Params = "
        "'0.1, -0.04, 2.9', StartOverlaps = '1.4, 0.1, 1.4', EndOverlaps = "
        "'1.6, 2.9, 1.6')",
        ""};

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, boost::get<0>(output), boost::is_any_of("\n"));

    int i = 0;
    for (const auto &line : notebookLines) {
      TS_ASSERT_EQUALS(line, result[i++])
    }

    // All rows in second group

    rowData0 = {"24681", "", "", "", "", "", "", ""};
    rowData1 = {"24682", "", "", "", "", "", "", ""};
    groupData = {{0, rowData0}, {1, rowData1}};
    output = postprocessGroupString(groupData, reflWhitelist(), reflProcessor(),
                                    reflPostprocessor(), userOptions);

    result = {"#Post-process workspaces",
              "IvsQ_TOF_24681_TOF_24682, _ = "
              "Stitch1DMany(InputWorkspaces = "
              "'IvsQ_binned_TOF_24681, IvsQ_binned_TOF_24682', Params = '0.1, "
              "-0.04, 2.9', StartOverlaps = '1.4, 0.1, 1.4', EndOverlaps = "
              "'1.6, 2.9, 1.6')",
              ""};

    boost::split(notebookLines, boost::get<0>(output), boost::is_any_of("\n"));

    i = 0;
    for (auto it = notebookLines.begin(); it != notebookLines.end();
         ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
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

  void testPlotsString() {
    std::vector<std::string> unprocessed_ws;
    unprocessed_ws.emplace_back("IvsQ_binned_1, IvsQ_1, IvsLam_1");
    unprocessed_ws.emplace_back("IvsQ_binned_2, IvsQ_2, IvsLam_2");

    std::vector<std::string> postprocessed_ws;
    postprocessed_ws.emplace_back("TEST_WS3");
    postprocessed_ws.emplace_back("TEST_WS4");

    std::string output = plotsString(
        unprocessed_ws, boost::algorithm::join(postprocessed_ws, "_"),
        reflProcessor());

    const std::string result[] = {
        "#Group workspaces to be plotted on same axes",
        "IvsQ_binned_groupWS = GroupWorkspaces(InputWorkspaces = "
        "'IvsQ_binned_1, IvsQ_binned_2')",
        "IvsQ_groupWS = GroupWorkspaces(InputWorkspaces = 'IvsQ_1, IvsQ_2')",
        "IvsLam_groupWS = GroupWorkspaces(InputWorkspaces = 'IvsLam_1, "
        "IvsLam_2')",
        "#Plot workspaces",
        "fig = plots([IvsQ_binned_groupWS, IvsQ_groupWS, IvsLam_groupWS, "
        "TEST_WS3_TEST_WS4], title=['IvsQ_binned_groupWS', 'IvsQ_groupWS', "
        "'IvsLam_groupWS', 'TEST_WS3_TEST_WS4'], legendLocation=[1, 1, 4])",
        ""};

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));
    std::cout << "\n";
    int i = 0;
    for (const auto &line : notebookLines) {
      TS_ASSERT_EQUALS(line, result[i++])
    }
  }

  void testPlotsStringNoPostprocessing() {
    // Reduced workspaces
    std::vector<std::string> unprocessed_ws;
    unprocessed_ws.emplace_back("IvsQ_binned_1, IvsQ_1, IvsLam_1");
    unprocessed_ws.emplace_back("IvsQ_binned_2, IvsQ_2, IvsLam_2");
    // Post-processed ws (empty)
    std::string postprocessed_ws;

    std::string output =
        plotsString(unprocessed_ws, postprocessed_ws, reflProcessor());

    const std::string result[] = {
        "#Group workspaces to be plotted on same axes",
        "IvsQ_binned_groupWS = GroupWorkspaces(InputWorkspaces = "
        "'IvsQ_binned_1, IvsQ_binned_2')",
        "IvsQ_groupWS = GroupWorkspaces(InputWorkspaces = 'IvsQ_1, IvsQ_2')",
        "IvsLam_groupWS = GroupWorkspaces(InputWorkspaces = 'IvsLam_1, "
        "IvsLam_2')",
        "#Plot workspaces",
        "fig = plots([IvsQ_binned_groupWS, IvsQ_groupWS, IvsLam_groupWS, ], "
        "title=['IvsQ_binned_groupWS', 'IvsQ_groupWS', 'IvsLam_groupWS', ''], "
        "legendLocation=[1, 1, 4])",
        ""};

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    int i = 0;
    for (const auto &line : notebookLines) {
      TS_ASSERT_EQUALS(line, result[i++])
    }
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

  void testGenerateNotebookReflectometry() {
    // A reflectometry case

    auto whitelist = reflWhitelist();
    auto preprocessMap = reflPreprocessMap();
    auto processor = reflProcessor();
    auto postProcessor = reflPostprocessor();
    auto preprocessingOptions = std::map<std::string, std::string>{
        {"Run(s)", "PlusProperty=PlusValue"},
        {"Transmission Run(s)", "Property=Value"}};
    auto processingOptions = "AnalysisMode=MultiDetectorAnalysis";
    auto postprocessingOptions = "Params=0.04";

    auto notebook = Mantid::Kernel::make_unique<DataProcessorGenerateNotebook>(
        "TableName", "INTER", whitelist, preprocessMap, processor,
        postProcessor, preprocessingOptions, processingOptions,
        postprocessingOptions);

    std::string generatedNotebook = notebook->generateNotebook(reflData());

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, generatedNotebook, boost::is_any_of("\n"));

    const std::string loadAndReduceStringFirstGroup =
        "               \"input\" : \"#Load and reduce\\n12345 = Load(Filename "
        "= \'INTER12345\')\\nIvsQ_binned_TOF_12345, IvsQ_TOF_12345, "
        "IvsLam_TOF_12345 = ReflectometryReductionOneAuto(InputWorkspace = "
        "\'12345\', ThetaIn = 0.5, MomentumTransferMin = 0.1, "
        "MomentumTransferMax = 1.6, MomentumTransferStep = 0.04, ScaleFactor = "
        "1, AnalysisMode = MultiDetectorAnalysis)\\n#Load and reduce\\n12346 = "
        "Load(Filename = \'INTER12346\')\\nIvsQ_binned_TOF_12346, "
        "IvsQ_TOF_12346, IvsLam_TOF_12346 = "
        "ReflectometryReductionOneAuto(InputWorkspace = \'12346\', ThetaIn = "
        "1.5, MomentumTransferMin = 1.4, MomentumTransferMax = 2.9, "
        "MomentumTransferStep = 0.04, ScaleFactor = 1, AnalysisMode = "
        "MultiDetectorAnalysis)\\n\",";
    TS_ASSERT_EQUALS(notebookLines[48], loadAndReduceStringFirstGroup);

    const std::string postProcessStringFirstGroup =
        "               \"input\" : \"#Post-process "
        "workspaces\\nIvsQ_TOF_12345_TOF_12346, _ = "
        "Stitch1DMany(InputWorkspaces = \'IvsQ_binned_TOF_12345, "
        "IvsQ_binned_TOF_12346\', "
        "Params=0.04)\",";
    TS_ASSERT_EQUALS(notebookLines[56], postProcessStringFirstGroup);

    const std::string groupWorkspacesStringFirstGroup =
        "               \"input\" : \"#Group workspaces to be plotted on same "
        "axes\\nIvsQ_binned_groupWS = GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_12345, IvsQ_binned_TOF_12346\')\\nIvsQ_groupWS = "
        "GroupWorkspaces(InputWorkspaces = \'IvsQ_TOF_12345, "
        "IvsQ_TOF_12346\')\\nIvsLam_groupWS = GroupWorkspaces(InputWorkspaces "
        "= \'IvsLam_TOF_12345, IvsLam_TOF_12346\')\\n#Plot workspaces\\nfig = "
        "plots([IvsQ_binned_groupWS, IvsQ_groupWS, IvsLam_groupWS, "
        "IvsQ_TOF_12345_TOF_12346], title=[\'IvsQ_binned_groupWS\', "
        "\'IvsQ_groupWS\', \'IvsLam_groupWS\', \'IvsQ_TOF_12345_TOF_12346\'], "
        "legendLocation=[1, 1, 4])\\n\",";
    ;
    TS_ASSERT_EQUALS(notebookLines[64], groupWorkspacesStringFirstGroup);

    const std::string loadAndReduceStringSecondGroup =
        "               \"input\" : \"#Load and reduce\\n24681 = Load(Filename "
        "= \'INTER24681\')\\nIvsQ_binned_TOF_24681, IvsQ_TOF_24681, "
        "IvsLam_TOF_24681 = ReflectometryReductionOneAuto(InputWorkspace = "
        "\'24681\', ThetaIn = 0.5, MomentumTransferMin = 0.1, "
        "MomentumTransferMax = 1.6, MomentumTransferStep = 0.04, ScaleFactor = "
        "1, AnalysisMode = MultiDetectorAnalysis)\\n#Load and reduce\\n24682 = "
        "Load(Filename = \'INTER24682\')\\nIvsQ_binned_TOF_24682, "
        "IvsQ_TOF_24682, IvsLam_TOF_24682 = "
        "ReflectometryReductionOneAuto(InputWorkspace = \'24682\', ThetaIn = "
        "1.5, MomentumTransferMin = 1.4, MomentumTransferMax = 2.9, "
        "MomentumTransferStep = 0.04, ScaleFactor = 1, AnalysisMode = "
        "MultiDetectorAnalysis)\\n\",";
    TS_ASSERT_EQUALS(notebookLines[77], loadAndReduceStringSecondGroup);

    const std::string postProcessStringSecondGroup =
        "               \"input\" : \"#Post-process "
        "workspaces\\nIvsQ_TOF_24681_TOF_24682, _ = "
        "Stitch1DMany(InputWorkspaces = \'IvsQ_binned_TOF_24681, "
        "IvsQ_binned_TOF_24682\', Params=0.04)\",";
    TS_ASSERT_EQUALS(notebookLines[85], postProcessStringSecondGroup);

    const std::string groupWorkspacesStringSecondGroup =
        "               \"input\" : \"#Group workspaces to be plotted on same "
        "axes\\nIvsQ_binned_groupWS = GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_24681, IvsQ_binned_TOF_24682\')\\nIvsQ_groupWS = "
        "GroupWorkspaces(InputWorkspaces = \'IvsQ_TOF_24681, "
        "IvsQ_TOF_24682\')\\nIvsLam_groupWS = GroupWorkspaces(InputWorkspaces "
        "= \'IvsLam_TOF_24681, IvsLam_TOF_24682\')\\n#Plot workspaces\\nfig = "
        "plots([IvsQ_binned_groupWS, IvsQ_groupWS, IvsLam_groupWS, "
        "IvsQ_TOF_24681_TOF_24682], title=[\'IvsQ_binned_groupWS\', "
        "\'IvsQ_groupWS\', \'IvsLam_groupWS\', \'IvsQ_TOF_24681_TOF_24682\'], "
        "legendLocation=[1, 1, 4])\\n\",";
    ;
    TS_ASSERT_EQUALS(notebookLines[93], groupWorkspacesStringSecondGroup);

    // Total number of lines
    TS_ASSERT_EQUALS(notebookLines.size(), 104);
  }

  void testGenerateNotebookReflectometryNoPostProcessing() {

    auto whitelist = reflWhitelist();
    auto preprocessMap = reflPreprocessMap();
    auto processor = reflProcessor();
    auto postProcessor = reflPostprocessor();
    auto preprocessingOptions = std::map<std::string, std::string>{
        {"Run(s)", "PlusProperty=PlusValue"},
        {"Transmission Run(s)", "Property=Value"}};
    auto processingOptions = "AnalysisMode=MultiDetectorAnalysis";
    auto postprocessingOptions = "Params=0.04";

    auto notebook = Mantid::Kernel::make_unique<DataProcessorGenerateNotebook>(
        "TableName", "INTER", whitelist, preprocessMap, processor,
        postProcessor, preprocessingOptions, processingOptions,
        postprocessingOptions);

    RowData rowData0 = {"12345", "0.5", "", "0.1", "1.6", "0.04", "1", ""};
    RowData rowData1 = {"12346", "1.5", "", "1.4", "2.9", "0.04", "1", ""};
    TreeData treeData = {{0, {{0, rowData0}}}, {1, {{0, rowData1}}}};

    std::string generatedNotebook = notebook->generateNotebook(treeData);

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, generatedNotebook, boost::is_any_of("\n"));

    // Only 75 lines because we only analyzed the first two runs
    TS_ASSERT_EQUALS(notebookLines.size(), 104);

    // First group

    std::string loadAndReduceString =
        "               \"input\" : \"#Load and reduce\\n12345 = Load(Filename "
        "= \'INTER12345\')\\nIvsQ_binned_TOF_12345, IvsQ_TOF_12345, "
        "IvsLam_TOF_12345 = ReflectometryReductionOneAuto(InputWorkspace = "
        "\'12345\', ThetaIn = 0.5, MomentumTransferMin = 0.1, "
        "MomentumTransferMax = 1.6, MomentumTransferStep = 0.04, ScaleFactor = "
        "1, AnalysisMode = MultiDetectorAnalysis)\\n\",";
    TS_ASSERT_EQUALS(notebookLines[48], loadAndReduceString);

    std::string postProcessString = "               \"input\" : \"\",";
    TS_ASSERT_EQUALS(notebookLines[56], postProcessString);

    std::string groupWorkspacesString =
        "               \"input\" : \"#Group workspaces to be plotted on same "
        "axes\\nIvsQ_binned_groupWS = GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_12345\')\\nIvsQ_groupWS = "
        "GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_TOF_12345\')\\nIvsLam_groupWS = "
        "GroupWorkspaces(InputWorkspaces = \'IvsLam_TOF_12345\')\\n#Plot "
        "workspaces\\nfig = plots([IvsQ_binned_groupWS, IvsQ_groupWS, "
        "IvsLam_groupWS, ], title=[\'IvsQ_binned_groupWS\', \'IvsQ_groupWS\', "
        "\'IvsLam_groupWS\', \'\'], legendLocation=[1, 1, 4])\\n\",";
    TS_ASSERT_EQUALS(notebookLines[64], groupWorkspacesString);

    // Second group

    loadAndReduceString =
        "               \"input\" : \"#Load and reduce\\n12346 = Load(Filename "
        "= \'INTER12346\')\\nIvsQ_binned_TOF_12346, IvsQ_TOF_12346, "
        "IvsLam_TOF_12346 = ReflectometryReductionOneAuto(InputWorkspace = "
        "\'12346\', ThetaIn = 1.5, MomentumTransferMin = 1.4, "
        "MomentumTransferMax = 2.9, MomentumTransferStep = 0.04, ScaleFactor = "
        "1, AnalysisMode = MultiDetectorAnalysis)\\n\",";
    TS_ASSERT_EQUALS(notebookLines[77], loadAndReduceString);

    postProcessString = "               \"input\" : \"\",";
    TS_ASSERT_EQUALS(notebookLines[85], postProcessString);

    groupWorkspacesString =
        "               \"input\" : \"#Group workspaces to be plotted on same "
        "axes\\nIvsQ_binned_groupWS = GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_12346\')\\nIvsQ_groupWS = "
        "GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_TOF_12346\')\\nIvsLam_groupWS = "
        "GroupWorkspaces(InputWorkspaces = \'IvsLam_TOF_12346\')\\n#Plot "
        "workspaces\\nfig = plots([IvsQ_binned_groupWS, IvsQ_groupWS, "
        "IvsLam_groupWS, ], title=[\'IvsQ_binned_groupWS\', \'IvsQ_groupWS\', "
        "\'IvsLam_groupWS\', \'\'], legendLocation=[1, 1, 4])\\n\",";
    TS_ASSERT_EQUALS(notebookLines[93], groupWorkspacesString);
  }
};

#endif // MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H
