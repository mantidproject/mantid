// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
#include "MantidQtWidgets/Common/DataProcessorUI/WorkspaceNameUtils.h"
#include "MantidTestHelpers/DataProcessorTestHelper.h"

using namespace DataProcessorTestHelper;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace Mantid::API;
using namespace testing;

class GenerateNotebookTest : public CxxTest::TestSuite {

private:
  // Creates a map with pre-processing instruction for reflectometry
  std::map<QString, PreprocessingAlgorithm>
  reflPreprocessMap(const QString &plusPrefix = "",
                    const QString &transPrefix = "TRANS_") {

    // Reflectometry pre-process map
    return std::map<QString, PreprocessingAlgorithm>{
        {"Run(s)",
         PreprocessingAlgorithm("Plus", plusPrefix, "+", std::set<QString>())},
        {"Transmission Run(s)",
         PreprocessingAlgorithm(
             "CreateTransmissionWorkspaceAuto", transPrefix, "_",
             std::set<QString>{"FirstTransmissionRun", "SecondTransmissionRun",
                               "OutputWorkspace"})}};
  }

  // Creates a reflectometry processing algorithm
  ProcessingAlgorithm reflProcessor() {

    return ProcessingAlgorithm(
        "ReflectometryReductionOneAuto",
        std::vector<QString>{"IvsQ_binned_", "IvsQ_", "IvsLam_"}, 1,
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
    // Create some rows in 2 groups
    TreeData treeData;
    treeData[0][0] =
        makeRowData({"12345", "0.5", "", "0.1", "1.6", "0.04", "1", "", ""});
    treeData[0][1] =
        makeRowData({"12346", "1.5", "", "1.4", "2.9", "0.04", "1", "", ""});
    treeData[1][0] =
        makeRowData({"24681", "0.5", "", "0.1", "1.6", "0.04", "1", "", ""});
    treeData[1][1] =
        makeRowData({"24682", "1.5", "", "1.4", "2.9", "0.04", "1", "", ""});

    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
        addPropertyValue(treeData[i][j], "AnalysisMode",
                         "MultiDetectorAnalysis");

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
      TS_ASSERT_EQUALS(expectedLines[i].toStdString(), line.toStdString());
      i++;
    }
  }

  static void
  assertContainsMatchingLines(std::vector<QString> const &expectedLines,
                              QString const &book) {
    auto lines = splitIntoLines(book);
    auto i = 0u;
    for (auto const &line : lines) {
      TS_ASSERT_EQUALS(expectedLines[i].toStdString(), line.toStdString());
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

    auto notebook = std::make_unique<GenerateNotebook>(
        m_wsName, m_instrument, reflWhitelist(),
        std::map<QString, PreprocessingAlgorithm>(), reflProcessor(),
        PostprocessingStep("", reflPostprocessor(), OptionsMap()),
        ColumnOptionsMap());

    auto generatedNotebook = notebook->generateNotebook(TreeData());

    auto notebookLines = splitIntoLines(generatedNotebook);
    const QString result[] = {
        "{",
        "   \"metadata\" : {",
        R"(      "name" : "Mantid Notebook")",
        "   },",
        "   \"nbformat\" : 3,",
        "   \"nbformat_minor\" : 0,",
        "   \"worksheets\" : [",
        "      {",
        "         \"cells\" : [",
        "            {",
        R"(               "cell_type" : "markdown",)",
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
    auto rowData =
        makeRowData({"24682", "1.5", "", "1.4", "2.9", "0.04", "1", "", ""});
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
        "1 | 24682 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 |  | ",
        ""};

    assertContainsMatchingLines(result, output);
  }

  void testLoadRunString() {
    auto output = loadRunString("12345", m_instrument, "TOF_");
    auto const result = std::string(
        "Load(Filename = 'INSTRUMENT12345', OutputWorkspace = 'TOF_12345')\n");
    TS_ASSERT_EQUALS(boost::get<0>(output).toStdString(), result)
  }

  void testPreprocessString() {

    auto reflectometryPreprocessMap = reflPreprocessMap();
    auto output = preprocessString("OUTPUT_WS", "INPUT_WS", "OUTPUT_WS",
                                   reflectometryPreprocessMap["Run(s)"], "");
    auto const result = std::string(
        "Plus(LHSWorkspace = 'OUTPUT_WS', "
        "RHSWorkspace = 'INPUT_WS', OutputWorkspace = 'OUTPUT_WS')\n");
    TS_ASSERT_EQUALS(output.toStdString(), result)
  }

  void testPreprocessStringWithOptions() {

    auto preprocessMap = reflPreprocessMap();
    auto transProcessor = preprocessMap["Transmission Run(s)"];
    auto output =
        preprocessString("OUTPUT_WS", "INPUT_WS", "OUTPUT_WS", transProcessor,
                         "WavelengthMin = 0.5, WavelengthMax = 5.0");
    auto result = std::string(
        "CreateTransmissionWorkspaceAuto(FirstTransmissionRun "
        "= 'OUTPUT_WS', SecondTransmissionRun = 'INPUT_WS', WavelengthMin = "
        "0.5, WavelengthMax = 5.0, OutputWorkspace = 'OUTPUT_WS')\n");
    TS_ASSERT_EQUALS(output.toStdString(), result)
  }

  void testLoadWorkspaceStringOneRun() {

    auto processor = reflPreprocessMap()["Transmission Run(s)"];
    auto output = loadWorkspaceString("RUN", "INST_", processor, "");
    TS_ASSERT_EQUALS(boost::get<1>(output), "TRANS_RUN");
    TS_ASSERT_EQUALS(
        boost::get<0>(output),
        "Load(Filename = 'INST_RUN', OutputWorkspace = 'TRANS_RUN')\n");
  }

  void testLoadWorkspaceStringThreeRunsWithOptions() {
    PreprocessingAlgorithm preprocessor("WeightedMean", "", "+");
    auto output = loadWorkspaceString("RUN1+RUN2,RUN3", "INST_", preprocessor,
                                      "Property1 = 1, Property2 = 2");
    auto outputLines = splitIntoLines(boost::get<0>(output));

    // The python code that does the loading
    const std::string result[] = {
        "Load(Filename = 'INST_RUN1', OutputWorkspace = 'RUN1+RUN2+RUN3')",
        "Load(Filename = 'INST_RUN2', OutputWorkspace = 'RUN2')",
        "WeightedMean(InputWorkspace1 = 'RUN1+RUN2+RUN3', "
        "InputWorkspace2 = 'RUN2', Property1 = 1, Property2 = 2, "
        "OutputWorkspace = 'RUN1+RUN2+RUN3')",
        "Load(Filename = 'INST_RUN3', OutputWorkspace = 'RUN3')",
        "WeightedMean(InputWorkspace1 = 'RUN1+RUN2+RUN3', "
        "InputWorkspace2 = 'RUN3', Property1 = 1, Property2 = 2, "
        "OutputWorkspace = 'RUN1+RUN2+RUN3')"};
    for (int i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(outputLines[i].toStdString(), result[i]);
    }

    // The loaded workspace
    TS_ASSERT_EQUALS(boost::get<1>(output).toStdString(), "RUN1+RUN2+RUN3");
  }

  void testReduceRowStringWrongData() {
    // Whitelist and data differ in size

    auto rowData = makeRowData({"12345", "1.5"});

    TS_ASSERT_THROWS_ANYTHING(reduceRowString(
        rowData, m_instrument, reflWhitelist(), reflPreprocessMap("TOF_"),
        reflProcessor(), ColumnOptionsMap()));
  }

  void testReduceRowString() {
    // Reduce a single row, no pre-processing is needed because there's
    // only one run in the 'Run(s)' column and no transmission runs

    ColumnOptionsMap userPreProcessingOptions = {
        {"Run(s)", OptionsMap()}, {"Transmission Run(s)", OptionsMap()}};

    // Create a row
    const auto rowData =
        makeRowData({"12346", "1.5", "", "1.4", "2.9", "0.04", "1", "", ""});
    addPropertyValue(rowData, "AnalysisMode", "MultiDetectorAnalysis");

    auto output = reduceRowString(rowData, m_instrument, reflWhitelist(),
                                  reflPreprocessMap("TOF_"), reflProcessor(),
                                  userPreProcessingOptions);

    const QString result[] = {
        "Load(Filename = 'INSTRUMENT12346', OutputWorkspace = 'TOF_12346')",
        "ReflectometryReductionOneAuto(AnalysisMode = 'MultiDetectorAnalysis', "
        "InputWorkspace = 'TOF_12346', "
        "MomentumTransferMax = '2.9', MomentumTransferMin = '1.4', "
        "MomentumTransferStep = '0.04', OutputWorkspace = 'IvsQ_TOF_12346', "
        "OutputWorkspaceBinned = 'IvsQ_binned_TOF_12346', "
        "OutputWorkspaceWavelength = 'IvsLam_TOF_12346', ScaleFactor = '1', "
        "ThetaIn = '1.5')",
        ""};

    assertContainsMatchingLines(result, output);
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
        {"Run",
         PreprocessingAlgorithm("Plus", "RUN_", "+", std::set<QString>())}};
    // Specify some pre-processing options
    auto runOptions = OptionsMap{{"Property", "prop"}};
    auto userPreProcessingOptions = ColumnOptionsMap{{"Run", runOptions}};

    // Create a row
    const auto data = makeRowData({"1000+1001", "0.5", "", "", "", "", "", ""});
    addPropertyValue(data, "AnalysisMode", "MultiDetectorAnalysis");

    // Set the expected output properties (these include the angle as specified
    // in the whitelist)
    addPropertyValue(data, "OutputWorkspace", "IvsQ_1000+1001_angle_0.5");
    addPropertyValue(data, "OutputWorkspaceBinned",
                     "IvsQ_binned_1000+1001_angle_0.5");
    addPropertyValue(data, "OutputWorkspaceWavelength",
                     "IvsLam_1000+1001_angle_0.5");

    auto output = reduceRowString(data, "INST", whitelist, preprocessMap,
                                  reflProcessor(), userPreProcessingOptions);

    const QString result[] = {
        "Load(Filename = 'INST1000', OutputWorkspace = 'RUN_1000+1001')",
        "Load(Filename = 'INST1001', OutputWorkspace = 'RUN_1001')",
        "Plus(LHSWorkspace = 'RUN_1000+1001', RHSWorkspace = "
        "'RUN_1001', Property='prop', OutputWorkspace = 'RUN_1000+1001')",
        "ReflectometryReductionOneAuto(AnalysisMode = 'MultiDetectorAnalysis', "
        "InputWorkspace = 'RUN_1000+1001', "
        "OutputWorkspace = 'IvsQ_1000+1001_angle_0.5', OutputWorkspaceBinned = "
        "'IvsQ_binned_1000+1001_angle_0.5', OutputWorkspaceWavelength = "
        "'IvsLam_1000+1001_angle_0.5', "
        "ThetaIn = '0.5')",
        ""};

    std::cout << output.toStdString() << std::endl;

    // Check the python code
    assertContainsMatchingLines(result, output);
  }

  void testReduceRowStringNoPreProcessing() {
    // Reduce a run without pre-processing algorithm specified (i.e. empty
    // pre-process map)

    std::map<QString, PreprocessingAlgorithm> emptyPreProcessMap;
    ColumnOptionsMap emptyPreProcessingOptions;

    // Create a row
    const auto data =
        makeRowData({"12346", "1.5", "", "1.4", "2.9", "0.04", "1", "", ""});
    addPropertyValue(data, "AnalysisMode", "MultiDetectorAnalysis");

    auto output =
        reduceRowString(data, m_instrument, reflWhitelist(), emptyPreProcessMap,
                        reflProcessor(), emptyPreProcessingOptions);

    const QString result[] = {
        "ReflectometryReductionOneAuto(AnalysisMode = 'MultiDetectorAnalysis', "
        "InputWorkspace = '12346', "
        "MomentumTransferMax = '2.9', MomentumTransferMin = '1.4', "
        "MomentumTransferStep = '0.04', OutputWorkspace = 'IvsQ_TOF_12346', "
        "OutputWorkspaceBinned = 'IvsQ_binned_TOF_12346', "
        "OutputWorkspaceWavelength = 'IvsLam_TOF_12346', ScaleFactor = '1', "
        "ThetaIn = '1.5')",
        ""};

    assertContainsMatchingLines(result, output);
  }

  void testReducedWorkspaceNameWrong() {
    // Whitelist and data differ in size

    // Create a whitelist
    WhiteList whitelist;
    whitelist.addElement("Run", "", "", true, "run_");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Trans", "", "", false, "");

    // Create some data
    const auto data = makeRowData(
        {"1000,1001", "0.5", "2000,2001", "1.4", "2.9", "0.04", "1", "", ""});
    auto reflectometryPreprocessMap = reflPreprocessMap();
    TS_ASSERT_THROWS_ANYTHING(
        getReducedWorkspaceName(data, whitelist, reflectometryPreprocessMap));
  }

  void testReducedWorkspaceNameOnlyRun() {

    // Create a whitelist
    WhiteList whitelist;
    whitelist.addElement("Run(s)", "", "", true, "run_");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Transmission Run(s)", "", "", false, "");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    whitelist.addElement("HiddenOptions", "HiddenOptions", "");

    // Create some data
    const auto data = makeRowData(
        {"1000,1001", "0.5", "2000,2001", "1.4", "2.9", "0.04", "1", "", ""});

    auto reflectometryPreprocessMap = reflPreprocessMap("run_", "");
    auto name =
        getReducedWorkspaceName(data, whitelist, reflectometryPreprocessMap);
    TS_ASSERT_EQUALS(name.toStdString(), "run_1000+1001")
  }

  void testReducedWorkspaceNameRunAndTrans() {

    // Create a whitelist
    WhiteList whitelist;
    whitelist.addElement("Run(s)", "", "", true, "run_");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Transmission Run(s)", "", "", true, "trans_");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    whitelist.addElement("HiddenOptions", "HiddenOptions", "");

    // Create some data
    const auto data = makeRowData(
        {"1000,1001", "0.5", "2000,2001", "1.4", "2.9", "0.04", "1", "", ""});

    auto reflectometryPreprocessMap = reflPreprocessMap("run_", "trans_");
    auto name =
        getReducedWorkspaceName(data, whitelist, reflectometryPreprocessMap);
    TS_ASSERT_EQUALS(name.toStdString(), "run_1000+1001_trans_2000_2001")
  }

  void testReducedWorkspaceNameTransNoPrefix() {

    // Create a whitelist
    WhiteList whitelist;
    whitelist.addElement("Run(s)", "", "", false, "");
    whitelist.addElement("Angle", "", "", false, "");
    whitelist.addElement("Transmission Run(s)", "", "", true, "");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    whitelist.addElement("HiddenOptions", "HiddenOptions", "");

    const auto data = makeRowData(
        {"1000,1001", "0.5", "2000+2001", "1.4", "2.9", "0.04", "1", "", ""});

    auto reflectometryPreprocessMap = reflPreprocessMap("", "");
    auto name =
        getReducedWorkspaceName(data, whitelist, reflectometryPreprocessMap);
    TS_ASSERT_EQUALS(name.toStdString(), "2000_2001")
  }

  void testPostprocessGroupString() {
    auto userOptions = "Params = '0.1, -0.04, 2.9', StartOverlaps = "
                       "'1.4, 0.1, 1.4', EndOverlaps = '1.6, 2.9, 1.6'";

    auto rowData0 = makeRowData({"12345", "", "", "", "", "", "", "", ""});
    auto rowData1 = makeRowData({"12346", "", "", "", "", "", "", "", ""});
    GroupData groupData = {{0, rowData0}, {1, rowData1}};

    auto output = postprocessGroupString(
        groupData, reflProcessor(),
        PostprocessingStep(userOptions, reflPostprocessor(), OptionsMap()));

    std::vector<QString> result = {
        "#Post-process workspaces",
        "Stitch1DMany(InputWorkspaces = "
        "'IvsQ_binned_TOF_12345, IvsQ_binned_TOF_12346', Params = "
        "'0.1, -0.04, 2.9', StartOverlaps = '1.4, 0.1, 1.4', EndOverlaps = "
        "'1.6, 2.9, 1.6', OutputWorkspace = 'IvsQ_TOF_12345_TOF_12346')",
        ""};

    assertContainsMatchingLines(result, boost::get<0>(output));
    // All rows in second group

    rowData0 = makeRowData({"24681", "", "", "", "", "", "", "", ""});
    rowData1 = makeRowData({"24682", "", "", "", "", "", "", "", ""});
    groupData = {{0, rowData0}, {1, rowData1}};
    output = postprocessGroupString(
        groupData, reflProcessor(),
        PostprocessingStep(userOptions, reflPostprocessor(), OptionsMap()));

    result = {"#Post-process workspaces",
              "Stitch1DMany(InputWorkspaces = "
              "'IvsQ_binned_TOF_24681, IvsQ_binned_TOF_24682', Params = '0.1, "
              "-0.04, 2.9', StartOverlaps = '1.4, 0.1, 1.4', EndOverlaps = "
              "'1.6, 2.9, 1.6', OutputWorkspace = 'IvsQ_TOF_24681_TOF_24682')",
              ""};

    assertContainsMatchingLines(result, boost::get<0>(output));
  }

  void testPlot1DString() {
    QStringList ws_names;
    ws_names.append("workspace1");
    ws_names.append("workspace2");

    auto output = plot1DString(ws_names);
    auto const result = std::string(
        "fig = plots([mtd['workspace1'], mtd['workspace2']], "
        "title=['workspace1', 'workspace2'], legendLocation=[1, 1])\n");

    TS_ASSERT_EQUALS(result, output.toStdString());
  }

  void testPlotsString() {
    // Reduced workspaces
    // Create a group with two rows and some dummy run numbers (with no
    // prefixes)
    auto rowData1 = makeRowData({"1"}, {});
    auto rowData2 = makeRowData({"2"}, {});
    auto groupData = GroupData();
    groupData[0] = rowData1;
    groupData[1] = rowData2;

    QStringList postprocessed_ws;
    postprocessed_ws.append("TEST_WS3");
    postprocessed_ws.append("TEST_WS4");

    auto output =
        plotsString(groupData, postprocessed_ws.join("_"), reflProcessor());

    const QString result[] = {
        "#Group workspaces to be plotted on same axes",
        "GroupWorkspaces(InputWorkspaces = "
        "'IvsQ_binned_1, IvsQ_binned_2', OutputWorkspace = "
        "'IvsQ_binned_groupWS')",
        "GroupWorkspaces(InputWorkspaces = 'IvsQ_1, "
        "IvsQ_2', OutputWorkspace = 'IvsQ_groupWS')",
        "GroupWorkspaces(InputWorkspaces = 'IvsLam_1, "
        "IvsLam_2', OutputWorkspace = 'IvsLam_groupWS')",
        "#Plot workspaces",
        "fig = plots([mtd['IvsQ_binned_groupWS'], mtd['IvsQ_groupWS'], "
        "mtd['IvsLam_groupWS'], "
        "mtd['TEST_WS3_TEST_WS4']], title=['IvsQ_binned_groupWS', "
        "'IvsQ_groupWS', "
        "'IvsLam_groupWS', 'TEST_WS3_TEST_WS4'], legendLocation=[1, 1, 4, 1])",
        ""};

    assertContainsMatchingLines(result, output);
  }

  void testPlotsStringNoPostprocessing() {
    // Reduced workspaces
    // Create a group with two rows and some dummy run numbers (with no
    // prefixes)
    auto rowData1 = makeRowData({"1"}, {});
    auto rowData2 = makeRowData({"2"}, {});
    auto groupData = GroupData();
    groupData[0] = rowData1;
    groupData[1] = rowData2;
    // Post-processed ws (empty)
    auto postprocessed_ws = "";

    auto output = plotsString(groupData, postprocessed_ws, reflProcessor());

    const QString result[] = {"#Group workspaces to be plotted on same axes",
                              "GroupWorkspaces(InputWorkspaces = "
                              "'IvsQ_binned_1, IvsQ_binned_2', OutputWorkspace "
                              "= 'IvsQ_binned_groupWS')",
                              "GroupWorkspaces(InputWorkspaces = 'IvsQ_1, "
                              "IvsQ_2', OutputWorkspace = 'IvsQ_groupWS')",
                              "GroupWorkspaces(InputWorkspaces = 'IvsLam_1, "
                              "IvsLam_2', OutputWorkspace = 'IvsLam_groupWS')",
                              "#Plot workspaces",
                              "fig = plots([mtd['IvsQ_binned_groupWS'], "
                              "mtd['IvsQ_groupWS'], mtd['IvsLam_groupWS']], "
                              "title=['IvsQ_binned_groupWS', 'IvsQ_groupWS', "
                              "'IvsLam_groupWS'], "
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

    // Test string list output is correct for vector of strings and vector
    // of
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
    auto runOptions = OptionsMap{{"PlusProperty", "PlusValue"}};
    auto transmissionOptions = OptionsMap{{"Property", "Value"}};
    auto preprocessingOptions = ColumnOptionsMap{
        {"Run(s)", runOptions}, {"Transmission Run(s)", transmissionOptions}};
    auto postprocessingOptions = "Params=0.04";
    auto postprocessingStep =
        PostprocessingStep(postprocessingOptions, postProcessor, OptionsMap());

    auto notebook = std::make_unique<GenerateNotebook>(
        "TableName", "INTER", whitelist, preprocessMap, processor,
        postprocessingStep, preprocessingOptions);

    auto generatedNotebook = notebook->generateNotebook(reflData());

    auto notebookLines = splitIntoLines(generatedNotebook);
    auto const loadAndReduceStringFirstGroup = std::string(
        "               \"input\" : \"#Load and reduce\\n"
        "Load(Filename "
        "= \'INTER12345\', OutputWorkspace = '12345')\\n"
        "ReflectometryReductionOneAuto(AnalysisMode = 'MultiDetectorAnalysis', "
        "InputWorkspace = "
        "\'12345\', "
        "MomentumTransferMax = '1.6', MomentumTransferMin = '0.1', "
        "MomentumTransferStep = '0.04', "
        "OutputWorkspace = 'IvsQ_TOF_12345', OutputWorkspaceBinned = "
        "'IvsQ_binned_TOF_12345', OutputWorkspaceWavelength = "
        "'IvsLam_TOF_12345', ScaleFactor = '1', ThetaIn = '0.5')\\n#Load and "
        "reduce\\n"
        "Load(Filename = \'INTER12346\', OutputWorkspace = '12346')\\n"
        "ReflectometryReductionOneAuto(AnalysisMode = 'MultiDetectorAnalysis', "
        "InputWorkspace = \'12346\', MomentumTransferMax = '2.9', "
        "MomentumTransferMin = '1.4', "
        "MomentumTransferStep = '0.04', OutputWorkspace = 'IvsQ_TOF_12346', "
        "OutputWorkspaceBinned = 'IvsQ_binned_TOF_12346', "
        "OutputWorkspaceWavelength = 'IvsLam_TOF_12346', ScaleFactor = '1', "
        "ThetaIn = '1.5')\\n\",");
    TS_ASSERT_EQUALS(notebookLines[48].toStdString(),
                     loadAndReduceStringFirstGroup);

    auto const postProcessStringFirstGroup = QString(
        "               \"input\" : \"#Post-process "
        "workspaces\\n"
        "Stitch1DMany(InputWorkspaces = \'IvsQ_binned_TOF_12345, "
        "IvsQ_binned_TOF_12346\', "
        "Params=0.04, OutputWorkspace = 'IvsQ_TOF_12345_TOF_12346')\",");
    TS_ASSERT_EQUALS(notebookLines[56], postProcessStringFirstGroup);

    auto const groupWorkspacesStringFirstGroup = std::string(
        "               \"input\" : \"#Group workspaces to be plotted on "
        "same "
        "axes\\nGroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_12345, IvsQ_binned_TOF_12346\', OutputWorkspace = "
        "'IvsQ_binned_groupWS')\\n"
        "GroupWorkspaces(InputWorkspaces = \'IvsQ_TOF_12345, "
        "IvsQ_TOF_12346\', OutputWorkspace = 'IvsQ_groupWS')\\n"
        "GroupWorkspaces(InputWorkspaces "
        "= \'IvsLam_TOF_12345, IvsLam_TOF_12346\', OutputWorkspace = "
        "'IvsLam_groupWS')\\n#Plot "
        "workspaces\\nfig = "
        "plots([mtd['IvsQ_binned_groupWS'], mtd['IvsQ_groupWS'], "
        "mtd['IvsLam_groupWS'], "
        "mtd['IvsQ_TOF_12345_TOF_12346']], title=[\'IvsQ_binned_groupWS\', "
        "\'IvsQ_groupWS\', \'IvsLam_groupWS\', "
        "\'IvsQ_TOF_12345_TOF_12346\'], "
        "legendLocation=[1, 1, 4, 1])\\n\",");
    ;
    TS_ASSERT_EQUALS(notebookLines[64].toStdString(),
                     groupWorkspacesStringFirstGroup);

    auto const loadAndReduceStringSecondGroup = std::string(
        "               \"input\" : \"#Load and reduce\\n"
        "Load(Filename "
        "= \'INTER24681\', OutputWorkspace = '24681')\\n"
        "ReflectometryReductionOneAuto(AnalysisMode = 'MultiDetectorAnalysis', "
        "InputWorkspace = "
        "\'24681\', "
        "MomentumTransferMax = '1.6', MomentumTransferMin = '0.1', "
        "MomentumTransferStep = '0.04', "
        "OutputWorkspace = 'IvsQ_TOF_24681', OutputWorkspaceBinned = "
        "'IvsQ_binned_TOF_24681', OutputWorkspaceWavelength = "
        "'IvsLam_TOF_24681', ScaleFactor = '1', ThetaIn = '0.5')\\n#Load and "
        "reduce\\n"
        "Load(Filename = \'INTER24682\', OutputWorkspace = '24682')\\n"
        "ReflectometryReductionOneAuto(AnalysisMode = 'MultiDetectorAnalysis', "
        "InputWorkspace = \'24682\', MomentumTransferMax = '2.9', "
        "MomentumTransferMin = '1.4', "
        "MomentumTransferStep = '0.04', OutputWorkspace = 'IvsQ_TOF_24682', "
        "OutputWorkspaceBinned = 'IvsQ_binned_TOF_24682', "
        "OutputWorkspaceWavelength = 'IvsLam_TOF_24682', ScaleFactor = '1', "
        "ThetaIn = '1.5')\\n\",");
    TS_ASSERT_EQUALS(notebookLines[77].toStdString(),
                     loadAndReduceStringSecondGroup);

    auto const postProcessStringSecondGroup =
        std::string("               \"input\" : \"#Post-process "
                    "workspaces\\n"
                    "Stitch1DMany(InputWorkspaces = \'IvsQ_binned_TOF_24681, "
                    "IvsQ_binned_TOF_24682\', Params=0.04, OutputWorkspace = "
                    "'IvsQ_TOF_24681_TOF_24682')\",");
    TS_ASSERT_EQUALS(notebookLines[85].toStdString(),
                     postProcessStringSecondGroup);

    auto const groupWorkspacesStringSecondGroup = std::string(
        "               \"input\" : \"#Group workspaces to be plotted on "
        "same "
        "axes\\nGroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_24681, IvsQ_binned_TOF_24682\', OutputWorkspace = "
        "'IvsQ_binned_groupWS')\\n"
        "GroupWorkspaces(InputWorkspaces = \'IvsQ_TOF_24681, "
        "IvsQ_TOF_24682\', OutputWorkspace = 'IvsQ_groupWS')\\n"
        "GroupWorkspaces(InputWorkspaces "
        "= \'IvsLam_TOF_24681, IvsLam_TOF_24682\', OutputWorkspace = "
        "'IvsLam_groupWS')\\n#Plot "
        "workspaces\\nfig = "
        "plots([mtd['IvsQ_binned_groupWS'], mtd['IvsQ_groupWS'], "
        "mtd['IvsLam_groupWS'], "
        "mtd['IvsQ_TOF_24681_TOF_24682']], title=[\'IvsQ_binned_groupWS\', "
        "\'IvsQ_groupWS\', \'IvsLam_groupWS\', "
        "\'IvsQ_TOF_24681_TOF_24682\'], "
        "legendLocation=[1, 1, 4, 1])\\n\",");

    TS_ASSERT_EQUALS(notebookLines[93].toStdString(),
                     groupWorkspacesStringSecondGroup);

    // Total number of lines
    TS_ASSERT_EQUALS(notebookLines.size(), 104);
  }

  void testGenerateNotebookReflectometryNoPostProcessing() {

    auto whitelist = reflWhitelist();
    auto preprocessMap = reflPreprocessMap();
    auto processor = reflProcessor();
    auto postProcessor = reflPostprocessor();
    auto runOptions = OptionsMap{{"PlusProperty", "PlusValue"}};
    auto transmissionOptions = OptionsMap{{"Property", "Value"}};
    auto preprocessingOptions = ColumnOptionsMap{
        {"Run(s)", runOptions}, {"Transmission Run(s)", transmissionOptions}};
    auto postprocessingOptions = "Params=0.04";
    auto postprocessingStep =
        PostprocessingStep(postprocessingOptions, postProcessor, OptionsMap());

    auto notebook = std::make_unique<GenerateNotebook>(
        "TableName", "INTER", whitelist, preprocessMap, processor,
        postprocessingStep, preprocessingOptions);

    auto rowData0 =
        makeRowData({"12345", "0.5", "", "0.1", "1.6", "0.04", "1", "", ""});
    auto rowData1 =
        makeRowData({"12346", "1.5", "", "1.4", "2.9", "0.04", "1", "", ""});
    addPropertyValue(rowData0, "AnalysisMode", "MultiDetectorAnalysis");
    addPropertyValue(rowData1, "AnalysisMode", "MultiDetectorAnalysis");
    TreeData treeData = {{0, {{0, rowData0}}}, {1, {{0, rowData1}}}};

    auto generatedNotebook = notebook->generateNotebook(treeData);

    auto notebookLines = splitIntoLines(generatedNotebook);

    // Only 75 lines because we only analyzed the first two runs
    TS_ASSERT_EQUALS(notebookLines.size(), 104);

    // First group

    auto loadAndReduceString = std::string(
        "               \"input\" : \"#Load and reduce\\n"
        "Load(Filename "
        "= \'INTER12345\', OutputWorkspace = '12345')\\n"
        "ReflectometryReductionOneAuto(AnalysisMode = 'MultiDetectorAnalysis', "
        "InputWorkspace = "
        "\'12345\', "
        "MomentumTransferMax = '1.6', MomentumTransferMin = '0.1', "
        "MomentumTransferStep = '0.04', "
        "OutputWorkspace = 'IvsQ_TOF_12345', OutputWorkspaceBinned = "
        "'IvsQ_binned_TOF_12345', OutputWorkspaceWavelength = "
        "'IvsLam_TOF_12345', ScaleFactor = '1', ThetaIn = '0.5')\\n\",");
    TS_ASSERT_EQUALS(notebookLines[48].toStdString(), loadAndReduceString);

    auto postProcessString = QString(R"(               "input" : "",)");
    TS_ASSERT_EQUALS(notebookLines[56], postProcessString);

    auto groupWorkspacesString = std::string(
        "               \"input\" : \"#Group workspaces to be plotted on "
        "same "
        "axes\\nGroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_12345\', OutputWorkspace = 'IvsQ_binned_groupWS')\\n"
        "GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_TOF_12345\', OutputWorkspace = 'IvsQ_groupWS')\\n"
        "GroupWorkspaces(InputWorkspaces = \'IvsLam_TOF_12345\', "
        "OutputWorkspace = 'IvsLam_groupWS')\\n#Plot "
        "workspaces\\nfig = plots([mtd['IvsQ_binned_groupWS'], "
        "mtd['IvsQ_groupWS'], "
        "mtd['IvsLam_groupWS']], title=[\'IvsQ_binned_groupWS\', "
        "\'IvsQ_groupWS\', "
        "\'IvsLam_groupWS\'], legendLocation=[1, 1, 4])\\n\",");
    TS_ASSERT_EQUALS(notebookLines[64].toStdString(), groupWorkspacesString);

    // Second group

    loadAndReduceString =
        "               \"input\" : \"#Load and reduce\\n"
        "Load(Filename "
        "= \'INTER12346\', OutputWorkspace = '12346')\\n"
        "ReflectometryReductionOneAuto(AnalysisMode = 'MultiDetectorAnalysis', "
        "InputWorkspace = "
        "\'12346\', "
        "MomentumTransferMax = '2.9', MomentumTransferMin = '1.4', "
        "MomentumTransferStep = '0.04', "
        "OutputWorkspace = 'IvsQ_TOF_12346', OutputWorkspaceBinned = "
        "'IvsQ_binned_TOF_12346', OutputWorkspaceWavelength = "
        "'IvsLam_TOF_12346', ScaleFactor = '1', ThetaIn = '1.5')\\n\",";
    TS_ASSERT_EQUALS(notebookLines[77].toStdString(), loadAndReduceString);

    postProcessString = R"(               "input" : "",)";
    TS_ASSERT_EQUALS(notebookLines[85], postProcessString);

    groupWorkspacesString =
        "               \"input\" : \"#Group workspaces to be plotted on "
        "same "
        "axes\\nGroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_binned_TOF_12346\', OutputWorkspace = 'IvsQ_binned_groupWS')\\n"
        "GroupWorkspaces(InputWorkspaces = "
        "\'IvsQ_TOF_12346\', OutputWorkspace = 'IvsQ_groupWS')\\n"
        "GroupWorkspaces(InputWorkspaces = \'IvsLam_TOF_12346\', "
        "OutputWorkspace = 'IvsLam_groupWS')\\n#Plot "
        "workspaces\\nfig = plots([mtd['IvsQ_binned_groupWS'], "
        "mtd['IvsQ_groupWS'], "
        "mtd['IvsLam_groupWS']], title=[\'IvsQ_binned_groupWS\', "
        "\'IvsQ_groupWS\', "
        "\'IvsLam_groupWS\'], legendLocation=[1, 1, 4])\\n\",";
    TS_ASSERT_EQUALS(notebookLines[93].toStdString(), groupWorkspacesString);
  }
};

#endif // MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H
