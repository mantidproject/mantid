#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenerateNotebook.h"
#include "MantidQtWidgets/Common/DataProcessorUI/VectorString.h"

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace Mantid::API;
using namespace testing;

class GenerateNotebookTest : public CxxTest::TestSuite {

private:
  // Creates a map with pre-processing instruction for reflectometry
  std::map<QString, PreprocessingAlgorithm>
  reflPreprocessMap(const QString &plusPrefix = "") {

    // Reflectometry pre-process map
    return std::map<QString, PreprocessingAlgorithm>{
        {"Run(s)",
         PreprocessingAlgorithm("Plus", plusPrefix, std::set<QString>())},
        {"Transmission Run(s)",
         PreprocessingAlgorithm("CreateTransmissionWorkspaceAuto", "TRANS_",
                                std::set<QString>{"FirstTransmissionRun",
                                                  "SecondTransmissionRun",
                                                  "OutputWorkspace"})}};
  }

  // Creates a reflectometry processing algorithm
  ProcessingAlgorithm reflProcessor() {

    return ProcessingAlgorithm(
        "ReflectometryReductionOneAuto",
        std::vector<QString>{"IvsQ_binned_", "IvsQ_", "IvsLam_"},
        std::set<QString>{"ThetaIn", "ThetaOut", "InputWorkspace",
                          "OutputWorkspace", "OutputWorkspaceWavelength",
                          "FirstTransmissionRun", "SecondTransmissionRun"});
  }

  PostprocessingAlgorithm reflPostprocessor() {
    return PostprocessingAlgorithm(
        "Stitch1DMany", "IvsQ_",
        std::set<QString>{"InputWorkspaces", "OutputWorkspace"});
  }

  // Creates a reflectometry whitelist
  WhiteList reflWhitelist() {

    // Reflectometry white list
    WhiteList whitelist;
    whitelist.addElement("Run(s)", "InputWorkspace", "", true, "TOF_");
    whitelist.addElement("Angle", "ThetaIn", "");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun", "",
                         true, "TRANS_");
    whitelist.addElement("Q min", "MomentumTransferMin", "");
    whitelist.addElement("Q max", "MomentumTransferMax", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    whitelist.addElement("HiddenOptions", "HiddenOptions", "");
    return whitelist;
  }

  // Creates reflectometry data
  TreeData reflData() {

    TreeData treeData;
    RowData rowData;

    rowData = {"12345", "0.5", "", "0.1", "1.6", "0.04", "1", "", ""};
    treeData[0][0] = rowData;
    rowData = {"12346", "1.5", "", "1.4", "2.9", "0.04", "1", "", ""};
    treeData[0][1] = rowData;
    rowData = {"24681", "0.5", "", "0.1", "1.6", "0.04", "1", "", ""};
    treeData[1][0] = rowData;
    rowData = {"24682", "1.5", "", "1.4", "2.9", "0.04", "1", "", ""};
    treeData[1][1] = rowData;

    return treeData;
  }

  QString m_wsName;
  QString m_instrument;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GenerateNotebookTest *createSuite() {
    return new GenerateNotebookTest();
  }
  static void destroySuite(GenerateNotebookTest *suite) { delete suite; }

  static QStringList splitIntoLines(QString const &notebook) {
    return notebook.split("\n");
  }

  static void assertContainsMatchingLines(QString const *expectedLines,
                                          QString const &book) {
    auto lines = splitIntoLines(book);
    auto i = 0u;
    for (auto const &line : lines) {
      TS_ASSERT_EQUALS(expectedLines[i], line);
      i++;
    }
  }

  static void
  assertContainsMatchingLines(std::vector<QString> const &expectedLines,
                              QString const &book) {
    auto lines = splitIntoLines(book);
    auto i = 0u;
    for (auto const &line : lines) {
      TS_ASSERT_EQUALS(expectedLines[i], line);
      i++;
    }
  }

  GenerateNotebookTest() { FrameworkManager::Instance(); }

  // Create a notebook to test
  void setUp() override {
    m_wsName = "TESTWORKSPACE";
    m_instrument = "INSTRUMENT";
  }

  void testGenerateNotebookFirstLines() {

    auto notebook = Mantid::Kernel::make_unique<GenerateNotebook>(
        m_wsName, m_instrument, reflWhitelist(),
        std::map<QString, PreprocessingAlgorithm>(), reflProcessor(),
        PostprocessingStep("", reflPostprocessor(),
                           std::map<QString, QString>()),
        std::map<QString, QString>(), "");

    auto generatedNotebook = notebook->generateNotebook(TreeData());

    auto notebookLines = splitIntoLines(generatedNotebook);
    const QString result[] = {
        "{", "   \"metadata\" : {", "      \"name\" : \"Mantid Notebook\"",
        "   },", "   \"nbformat\" : 3,", "   \"nbformat_minor\" : 0,",
        "   \"worksheets\" : [", "      {", "         \"cells\" : [",
        "            {", "               \"cell_type\" : \"markdown\",",
    };

    // Check that the first 10 lines are output as expected
    for (auto i = 0u; i < 11u; ++i) {
      TS_ASSERT_EQUALS(notebookLines[i], result[i])
    }
  }

  void testTitleString() {
    // With workspace name
    auto output = titleString("TEST_WORKSPACE");
    const QString result[] = {"Processed data from workspace: TEST_WORKSPACE",
                              "---------------", ""};
    assertContainsMatchingLines(result, output);

    // Without workspace name
    auto outputEmptyStr = titleString("");
    const QString resultEmptyStr[] = {"Processed data", "---------------", ""};
    assertContainsMatchingLines(resultEmptyStr, outputEmptyStr);
  }

  void testTableStringWrongData() {
    // Whitelist and data incompatible

    WhiteList whitelist;
    whitelist.addElement("Run", "Run", "");
    whitelist.addElement("Angle", "Angle", "");

    TreeData treeData = reflData();

    TS_ASSERT_THROWS_ANYTHING(tableString(treeData, whitelist));
  }

  void testTableStringOneRow() {

    // Create some tree data
    RowData rowData = {"24682", "1.5", "", "1.4", "2.9",
                       "0.04",  "1",   "", "" /*Hidden Option*/};
    TreeData treeData = {{1, {{0, rowData}}}};

    auto output = tableString(treeData, reflWhitelist());

    const QString result[] = {
        "Group | Run(s) | Angle | Transmission Run(s) | Q min | Q max | dQ/Q | "
        "Scale | Options | HiddenOptions",
        "--- | --- | --- | --- | --- | --- | --- | "
        "--- | ---",
        "1 | 24682 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 |  | ", ""};

    assertContainsMatchingLines(result, output);
  }

  void testTableStringAllRows() {
    auto output = tableString(reflData(), reflWhitelist());
    const QString result[] = {
        "Group | Run(s) | Angle | Transmission Run(s) | Q min | Q max | dQ/Q | "
        "Scale | Options | HiddenOptions",
        "--- | --- | --- | --- | --- | --- | --- | "
        "--- | ---",
        "0 | 12345 | 0.5 |  | 0.1 | 1.6 | 0.04 | 1 |  | ",
        "0 | 12346 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 |  | ",
        "1 | 24681 | 0.5 |  | 0.1 | 1.6 | 0.04 | 1 |  | ",
        "1 | 24682 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 |  | ", ""};

    assertContainsMatchingLines(result, output);
  }

  void testLoadRunString() {
    auto output = loadRunString("12345", m_instrument, "TOF_");
    auto const result =
        QString("TOF_12345 = Load(Filename = 'INSTRUMENT12345')\n");
    TS_ASSERT_EQUALS(boost::get<0>(output), result)
  }

  void testPlusString() {

    auto reflectometryPreprocessMap = reflPreprocessMap();
    auto output = plusString("INPUT_WS", "OUTPUT_WS",
                             reflectometryPreprocessMap["Run(s)"], "");
    auto const result = QString("OUTPUT_WS = Plus(LHSWorkspace = 'OUTPUT_WS', "
                                "RHSWorkspace = 'INPUT_WS')\n");
    TS_ASSERT_EQUALS(output, result)
  }

  void testPlusStringWithOptions() {

    auto preprocessMap = reflPreprocessMap();
    auto transProcessor = preprocessMap["Transmission Run(s)"];
    auto output = plusString("INPUT_WS", "OUTPUT_WS", transProcessor,
                             "WavelengthMin = 0.5, WavelengthMax = 5.0");
    auto result = QString(
        "OUTPUT_WS = CreateTransmissionWorkspaceAuto(FirstTransmissionRun "
        "= 'OUTPUT_WS', SecondTransmissionRun = 'INPUT_WS', WavelengthMin = "
        "0.5, WavelengthMax = 5.0)\n");
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
    PreprocessingAlgorithm preprocessor("WeightedMean");
    auto output = loadWorkspaceString("RUN1+RUN2,RUN3", "INST_", preprocessor,
                                      "Property1 = 1, Property2 = 2");
    auto outputLines = splitIntoLines(boost::get<0>(output));

    // The python code that does the loading
    const QString result[] = {
        "RUN1 = Load(Filename = 'INST_RUN1')", "RUN1_RUN2_RUN3 = RUN1",
        "RUN2 = Load(Filename = 'INST_RUN2')",
        "RUN1_RUN2_RUN3 = WeightedMean(InputWorkspace1 = 'RUN1_RUN2_RUN3', "
        "InputWorkspace2 = 'RUN2', Property1 = 1, Property2 = 2)",
        "RUN3 = Load(Filename = 'INST_RUN3')",
        "RUN1_RUN2_RUN3 = WeightedMean(InputWorkspace1 = 'RUN1_RUN2_RUN3', "
        "InputWorkspace2 = 'RUN3', Property1 = 1, Property2 = 2)"};
    for (int i = 0; i < 6; i++) {
      TS_ASSERT_EQUALS(outputLines[i], result[i]);
    }

    // The loaded workspace
    TS_ASSERT_EQUALS(boost::get<1>(output), "RUN1_RUN2_RUN3");
  }

  void testReduceRowStringWrongData() {
    // Whitelist and data differ in size

    RowData rowData = {"12345", "1.5"};

    TS_ASSERT_THROWS_ANYTHING(reduceRowString(
        rowData, m_instrument, reflWhitelist(), reflPreprocessMap("TOF_"),
        reflProcessor(), std::map<QString, QString>(), ""));
  }

  void testReduceRowString() {
    // Reduce a single row, no pre-processing is needed because there's
    // only one run in the 'Run(s)' column and no transmission runs

    std::map<QString, QString> userPreProcessingOptions = {
        {"Run(s)", ""}, {"Transmission Run(s)", ""}};

    const RowData data = {"12346", "1.5", "", "1.4", "2.9",
                          "0.04",  "1",   "", ""};

    auto output = reduceRowString(data, m_instrument, reflWhitelist(),
                                  reflPreprocessMap("TOF_"), reflProcessor(),
                                  userPreProcessingOptions, "");

    const QString result[] = {
        "TOF_12346 = Load(Filename = 'INSTRUMENT12346')",
        "IvsQ_binned_TOF_12346, IvsQ_TOF_12346, IvsLam_TOF_12346 = "
        "ReflectometryReductionOneAuto(InputWorkspace = 'TOF_12346', ThetaIn = "
        "1.5, MomentumTransferMin = 1.4, MomentumTransferMax = 2.9, "
        "MomentumTransferStep = 0.04, ScaleFactor = 1)",
        ""};

    assertContainsMatchingLines(result, boost::get<0>(output));
  }

  void testReduceRowStringWithPreprocessing() {
    // Reduce a single row, one column need pre-processing

    // Create a whitelist
    WhiteList whitelist;
    whitelist.addElement("Run", "InputWorkspace", "", true);
    whitelist.addElement("Angle", "ThetaIn", "", true, "angle_");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun", "");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");

    // Create a pre-process map
    std::map<QString, PreprocessingAlgorithm> preprocessMap = {
        {"Run", PreprocessingAlgorithm("Plus", "RUN_", std::set<QString>())}};
    // Specify some pre-processing options
    std::map<QString, QString> userPreProcessingOptions = {
        {"Run", "Property=prop"}};

    // Create some data
    const RowData data = {"1000+1001", "0.5", "", "", "", "", "", ""};

    auto output =
        reduceRowString(data, "INST", whitelist, preprocessMap, reflProcessor(),
                        userPreProcessingOptions, "");

    const QString result[] = {
        "RUN_1000 = Load(Filename = 'INST1000')", "RUN_1000_1001 = RUN_1000",
        "RUN_1001 = Load(Filename = 'INST1001')",
        "RUN_1000_1001 = Plus(LHSWorkspace = 'RUN_1000_1001', RHSWorkspace = "
        "'RUN_1001', Property=prop)",
        "IvsQ_binned_1000_1001_angle_0.5, IvsQ_1000_1001_angle_0.5, "
        "IvsLam_1000_1001_angle_0.5 = "
        "ReflectometryReductionOneAuto(InputWorkspace = 'RUN_1000_1001', "
        "ThetaIn = 0.5)",
        ""};

    std::cout << boost::get<1>(output).toStdString() << std::endl;

    // Check the names of the reduced workspaces
    TS_ASSERT_EQUALS(boost::get<1>(output), "IvsQ_binned_1000_1001_angle_0.5, "
                                            "IvsQ_1000_1001_angle_0.5, "
                                            "IvsLam_1000_1001_angle_0.5");

    // Check the python code
    assertContainsMatchingLines(result, boost::get<0>(output));
  }

  void testReduceRowStringNoPreProcessing() {
    // Reduce a run without pre-processing algorithm specified (i.e. empty
    // pre-process map)

    std::map<QString, PreprocessingAlgorithm> emptyPreProcessMap;
    std::map<QString, QString> emptyPreProcessingOptions;

    const RowData data = {"12346", "1.5", "", "1.4", "2.9",
                          "0.04",  "1",   "", ""};

    auto output =
        reduceRowString(data, m_instrument, reflWhitelist(), emptyPreProcessMap,
                        reflProcessor(), emptyPreProcessingOptions, "");

    const QString result[] = {
        "IvsQ_binned_TOF_12346, IvsQ_TOF_12346, IvsLam_TOF_12346 = "
        "ReflectometryReductionOneAuto(InputWorkspace = 12346, ThetaIn = 1.5, "
        "MomentumTransferMin = 1.4, MomentumTransferMax = 2.9, "
        "MomentumTransferStep = 0.04, ScaleFactor = 1)",
        ""};

    assertContainsMatchingLines(result, boost::get<0>(output));
  }

  void testReducedWorkspaceNameWrong() {
    // Whitelist and data differ in size

    // Create a whitelist
    WhiteList whitelist;
    whitelist.addElement("Run", "", "", true, "run_");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Trans", "", "", false, "");

    // Create some data
    const RowData data = {"1000,1001", "0.5", "2000,2001", "1.4", "2.9",
                          "0.04",      "1",   "",          ""};

    TS_ASSERT_THROWS_ANYTHING(
        getReducedWorkspaceName(data, whitelist, "IvsQ_"));
  }

  void testReducedWorkspaceNameOnlyRun() {

    // Create a whitelist
    WhiteList whitelist;
    whitelist.addElement("Run", "", "", true, "run_");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Trans", "", "", false, "");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    whitelist.addElement("HiddenOptions", "HiddenOptions", "");

    // Create some data
    const RowData data = {"1000,1001", "0.5", "2000,2001", "1.4", "2.9",
                          "0.04",      "1",   "",          ""};

    auto name = getReducedWorkspaceName(data, whitelist, "IvsQ_");
    TS_ASSERT_EQUALS(name, "IvsQ_run_1000_1001")
  }

  void testReducedWorkspaceNameRunAndTrans() {

    // Create a whitelist
    WhiteList whitelist;
    whitelist.addElement("Run", "", "", true, "run_");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Trans", "", "", true, "trans_");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    whitelist.addElement("HiddenOptions", "HiddenOptions", "");

    // Create some data
    const RowData data = {"1000,1001", "0.5", "2000,2001", "1.4", "2.9",
                          "0.04",      "1",   "",          ""};

    auto name = getReducedWorkspaceName(data, whitelist, "Prefix_");
    TS_ASSERT_EQUALS(name, "Prefix_run_1000_1001_trans_2000_2001")
  }

  void testReducedWorkspaceNameTransNoPrefix() {

    // Create a whitelist
    WhiteList whitelist;
    whitelist.addElement("Run", "", "", false, "");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Trans", "", "", true, "");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    whitelist.addElement("HiddenOptions", "HiddenOptions", "");

    const RowData data = {"1000,1001", "0.5", "2000+2001", "1.4", "2.9",
                          "0.04",      "1",   "",          ""};

    auto name = getReducedWorkspaceName(data, whitelist, "Prefix_");
    TS_ASSERT_EQUALS(name, "Prefix_2000_2001")
  }

  void testPostprocessGroupString() {
    auto userOptions = "Params = '0.1, -0.04, 2.9', StartOverlaps = "
                       "'1.4, 0.1, 1.4', EndOverlaps = '1.6, 2.9, 1.6'";

    RowData rowData0 = {"12345", "", "", "", "", "", "", "", ""};
    RowData rowData1 = {"12346", "", "", "", "", "", "", "", ""};
    GroupData groupData = {{0, rowData0}, {1, rowData1}};

    auto output = postprocessGroupString(
        groupData, reflWhitelist(), reflProcessor(),
        PostprocessingStep(userOptions, reflPostprocessor(),
                           std::map<QString, QString>()));

    std::vector<QString> result = {
        "#Post-process workspaces",
        "IvsQ_TOF_12345_TOF_12346, _ = "
        "Stitch1DMany(InputWorkspaces = "
        "'IvsQ_binned_TOF_12345, IvsQ_binned_TOF_12346', Params = "
        "'0.1, -0.04, 2.9', StartOverlaps = '1.4, 0.1, 1.4', EndOverlaps = "
        "'1.6, 2.9, 1.6')",
        ""};

    assertContainsMatchingLines(result, boost::get<0>(output));
    // All rows in second group

    rowData0 = {"24681", "", "", "", "", "", "", "", ""};
    rowData1 = {"24682", "", "", "", "", "", "", "", ""};
    groupData = {{0, rowData0}, {1, rowData1}};
    output = postprocessGroupString(
        groupData, reflWhitelist(), reflProcessor(),
        PostprocessingStep(userOptions, reflPostprocessor(),
                           std::map<QString, QString>()));

    result = {"#Post-process workspaces",
              "IvsQ_TOF_24681_TOF_24682, _ = "
              "Stitch1DMany(InputWorkspaces = "
              "'IvsQ_binned_TOF_24681, IvsQ_binned_TOF_24682', Params = '0.1, "
              "-0.04, 2.9', StartOverlaps = '1.4, 0.1, 1.4', EndOverlaps = "
              "'1.6, 2.9, 1.6')",
              ""};

    assertContainsMatchingLines(result, boost::get<0>(output));
  }

  void testPlot1DString() {
    QStringList ws_names;
    ws_names.append("workspace1");
    ws_names.append("workspace2");

    auto output = plot1DString(ws_names);
    auto const result = QString(
        "fig = plots([workspace1, workspace2], "
        "title=['workspace1', 'workspace2'], legendLocation=[1, 1, 4])\n");

    TS_ASSERT_EQUALS(result, output);
  }

  void testPlotsString() {
    QStringList unprocessed_ws;
    unprocessed_ws.append("IvsQ_binned_1, IvsQ_1, IvsLam_1");
    unprocessed_ws.append("IvsQ_binned_2, IvsQ_2, IvsLam_2");

    QStringList postprocessed_ws;
    postprocessed_ws.append("TEST_WS3");
    postprocessed_ws.append("TEST_WS4");

    auto output = plotsString(unprocessed_ws, postprocessed_ws.join("_"),
                              reflProcessor());

    const QString result[] = {
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

    assertContainsMatchingLines(result, output);
  }

  void testPlotsStringNoPostprocessing() {
    // Reduced workspaces
    QStringList unprocessed_ws;
    unprocessed_ws.append("IvsQ_binned_1, IvsQ_1, IvsLam_1");
    unprocessed_ws.append("IvsQ_binned_2, IvsQ_2, IvsLam_2");
    // Post-processed ws (empty)
    auto postprocessed_ws = "";

    auto output =
        plotsString(unprocessed_ws, postprocessed_ws, reflProcessor());

    const QString result[] = {
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

    assertContainsMatchingLines(result, output);
  }

  void testVectorParamString() {
    std::vector<QString> stringVector;
    stringVector.emplace_back("A");
    stringVector.emplace_back("B");
    stringVector.emplace_back("C");

    auto const stringOutput = vectorParamString("PARAM_NAME", stringVector);

    TS_ASSERT_EQUALS(stringOutput, "PARAM_NAME = 'A, B, C'")
  }

  void testVectorString() {
    std::vector<QString> stringVector;
    stringVector.emplace_back("A");
    stringVector.emplace_back("B");
    stringVector.emplace_back("C");
    auto const stringOutput = vectorString(stringVector);

    std::vector<int> intVector;
    intVector.emplace_back(1);
    intVector.emplace_back(2);
    intVector.emplace_back(3);
    auto const intOutput = vectorString(intVector);

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
    auto preprocessingOptions =
        std::map<QString, QString>{{"Run(s)", "PlusProperty=PlusValue"},
                                   {"Transmission Run(s)", "Property=Value"}};
    auto processingOptions = "AnalysisMode=MultiDetectorAnalysis";
    auto postprocessingOptions = "Params=0.04";
    auto postprocessingStep = PostprocessingStep(
        postprocessingOptions, postProcessor, std::map<QString, QString>());

    auto notebook = Mantid::Kernel::make_unique<GenerateNotebook>(
        "TableName", "INTER", whitelist, preprocessMap, processor,
        postprocessingStep, preprocessingOptions, processingOptions);

    auto generatedNotebook = notebook->generateNotebook(reflData());

    auto notebookLines = splitIntoLines(generatedNotebook);
    auto const loadAndReduceStringFirstGroup = QString(
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
        "MultiDetectorAnalysis)\\n\",");
    TS_ASSERT_EQUALS(notebookLines[48], loadAndReduceStringFirstGroup);

    auto const postProcessStringFirstGroup =
        QString("               \"input\" : \"#Post-process "
                "workspaces\\nIvsQ_TOF_12345_TOF_12346, _ = "
                "Stitch1DMany(InputWorkspaces = \'IvsQ_binned_TOF_12345, "
                "IvsQ_binned_TOF_12346\', "
                "Params=0.04)\",");
    TS_ASSERT_EQUALS(notebookLines[56], postProcessStringFirstGroup);

    auto const groupWorkspacesStringFirstGroup = QString(
        "               \"input\" : \"#Group workspaces to be plotted on same "
        "axes\\nIvsQ_binned_groupWS = GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_12345, IvsQ_binned_TOF_12346\')\\nIvsQ_groupWS = "
        "GroupWorkspaces(InputWorkspaces = \'IvsQ_TOF_12345, "
        "IvsQ_TOF_12346\')\\nIvsLam_groupWS = GroupWorkspaces(InputWorkspaces "
        "= \'IvsLam_TOF_12345, IvsLam_TOF_12346\')\\n#Plot workspaces\\nfig = "
        "plots([IvsQ_binned_groupWS, IvsQ_groupWS, IvsLam_groupWS, "
        "IvsQ_TOF_12345_TOF_12346], title=[\'IvsQ_binned_groupWS\', "
        "\'IvsQ_groupWS\', \'IvsLam_groupWS\', \'IvsQ_TOF_12345_TOF_12346\'], "
        "legendLocation=[1, 1, 4])\\n\",");
    ;
    TS_ASSERT_EQUALS(notebookLines[64], groupWorkspacesStringFirstGroup);

    auto const loadAndReduceStringSecondGroup = QString(
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
        "MultiDetectorAnalysis)\\n\",");
    TS_ASSERT_EQUALS(notebookLines[77], loadAndReduceStringSecondGroup);

    auto const postProcessStringSecondGroup =
        QString("               \"input\" : \"#Post-process "
                "workspaces\\nIvsQ_TOF_24681_TOF_24682, _ = "
                "Stitch1DMany(InputWorkspaces = \'IvsQ_binned_TOF_24681, "
                "IvsQ_binned_TOF_24682\', Params=0.04)\",");
    TS_ASSERT_EQUALS(notebookLines[85], postProcessStringSecondGroup);

    auto const groupWorkspacesStringSecondGroup = QString(
        "               \"input\" : \"#Group workspaces to be plotted on same "
        "axes\\nIvsQ_binned_groupWS = GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_24681, IvsQ_binned_TOF_24682\')\\nIvsQ_groupWS = "
        "GroupWorkspaces(InputWorkspaces = \'IvsQ_TOF_24681, "
        "IvsQ_TOF_24682\')\\nIvsLam_groupWS = GroupWorkspaces(InputWorkspaces "
        "= \'IvsLam_TOF_24681, IvsLam_TOF_24682\')\\n#Plot workspaces\\nfig = "
        "plots([IvsQ_binned_groupWS, IvsQ_groupWS, IvsLam_groupWS, "
        "IvsQ_TOF_24681_TOF_24682], title=[\'IvsQ_binned_groupWS\', "
        "\'IvsQ_groupWS\', \'IvsLam_groupWS\', \'IvsQ_TOF_24681_TOF_24682\'], "
        "legendLocation=[1, 1, 4])\\n\",");

    TS_ASSERT_EQUALS(notebookLines[93], groupWorkspacesStringSecondGroup);

    // Total number of lines
    TS_ASSERT_EQUALS(notebookLines.size(), 104);
  }

  void testGenerateNotebookReflectometryNoPostProcessing() {

    auto whitelist = reflWhitelist();
    auto preprocessMap = reflPreprocessMap();
    auto processor = reflProcessor();
    auto postProcessor = reflPostprocessor();
    auto preprocessingOptions =
        std::map<QString, QString>{{"Run(s)", "PlusProperty=PlusValue"},
                                   {"Transmission Run(s)", "Property=Value"}};
    auto processingOptions = "AnalysisMode=MultiDetectorAnalysis";
    auto postprocessingOptions = "Params=0.04";
    auto postprocessingStep = PostprocessingStep(
        postprocessingOptions, postProcessor, std::map<QString, QString>());

    auto notebook = Mantid::Kernel::make_unique<GenerateNotebook>(
        "TableName", "INTER", whitelist, preprocessMap, processor,
        postprocessingStep, preprocessingOptions, processingOptions);

    RowData rowData0 = {"12345", "0.5", "", "0.1", "1.6", "0.04", "1", "", ""};
    RowData rowData1 = {"12346", "1.5", "", "1.4", "2.9", "0.04", "1", "", ""};
    TreeData treeData = {{0, {{0, rowData0}}}, {1, {{0, rowData1}}}};

    auto generatedNotebook = notebook->generateNotebook(treeData);

    auto notebookLines = splitIntoLines(generatedNotebook);

    // Only 75 lines because we only analyzed the first two runs
    TS_ASSERT_EQUALS(notebookLines.size(), 104);

    // First group

    auto loadAndReduceString = QString(
        "               \"input\" : \"#Load and reduce\\n12345 = Load(Filename "
        "= \'INTER12345\')\\nIvsQ_binned_TOF_12345, IvsQ_TOF_12345, "
        "IvsLam_TOF_12345 = ReflectometryReductionOneAuto(InputWorkspace = "
        "\'12345\', ThetaIn = 0.5, MomentumTransferMin = 0.1, "
        "MomentumTransferMax = 1.6, MomentumTransferStep = 0.04, ScaleFactor = "
        "1, AnalysisMode = MultiDetectorAnalysis)\\n\",");
    TS_ASSERT_EQUALS(notebookLines[48], loadAndReduceString);

    auto postProcessString = QString("               \"input\" : \"\",");
    TS_ASSERT_EQUALS(notebookLines[56], postProcessString);

    auto groupWorkspacesString = QString(
        "               \"input\" : \"#Group workspaces to be plotted on same "
        "axes\\nIvsQ_binned_groupWS = GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_12345\')\\nIvsQ_groupWS = "
        "GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_TOF_12345\')\\nIvsLam_groupWS = "
        "GroupWorkspaces(InputWorkspaces = \'IvsLam_TOF_12345\')\\n#Plot "
        "workspaces\\nfig = plots([IvsQ_binned_groupWS, IvsQ_groupWS, "
        "IvsLam_groupWS, ], title=[\'IvsQ_binned_groupWS\', \'IvsQ_groupWS\', "
        "\'IvsLam_groupWS\', \'\'], legendLocation=[1, 1, 4])\\n\",");
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
