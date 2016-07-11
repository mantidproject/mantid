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
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTreeModel.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace Mantid::Kernel;
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
        std::vector<std::string>{"IvsQ_", "IvsLam_"},
        std::set<std::string>{"ThetaIn", "ThetaOut", "InputWorkspace",
                              "OutputWorkspace", "OutputWorkspaceWavelength",
                              "FirstTransmissionRun", "SecondTransmissionRun"});
  }

  // Creates a reflectometry whitelist
  DataProcessorWhiteList reflWhitelist() {

    // Reflectometry white list
    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run(s)", "InputWorkspace", "", true, "TOF_");
    whitelist.addElement("Angle", "ThetaIn", "");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun", "",
                         true, "TRANS_");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    return whitelist;
  }

  // Creates a reflectometry table ws
  ITableWorkspace_sptr reflWorkspace() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "Group");
    ws->addColumn("str", "Run(s)");
    ws->addColumn("str", "Angle");
    ws->addColumn("str", "Transmission Run(s)");
    ws->addColumn("str", "MomentumTransferMinimum");
    ws->addColumn("str", "MomentumTransferMaximum");
    ws->addColumn("str", "MomentumTransferStep");
    ws->addColumn("str", "Scale");
    ws->addColumn("str", "Options");
    TableRow row = ws->appendRow();
    row << "0"
        << "12345"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "0"
        << "12346"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "1"
        << "24681"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "1"
        << "24682"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    return ws;
  }

  // Creates a reflectometry tree model
  QDataProcessorTreeModel_sptr reflModel() {
    return boost::shared_ptr<QDataProcessorTreeModel>(
        new QDataProcessorTreeModel(reflWorkspace(), reflWhitelist()));
  }

  std::string m_wsName;
  std::string m_instrument;
  QDataProcessorTreeModel_sptr m_model;

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
    m_model = reflModel();
  }

  void testGenerateNotebook() {

    auto notebook = Mantid::Kernel::make_unique<DataProcessorGenerateNotebook>(
        m_wsName, m_model, m_instrument, reflWhitelist(),
        std::map<std::string, DataProcessorPreprocessingAlgorithm>(),
        reflProcessor(), DataProcessorPostprocessingAlgorithm(),
        std::map<std::string, std::string>(), "", "");

    std::string generatedNotebook =
        notebook->generateNotebook(std::map<int, std::set<int>>());

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, generatedNotebook, boost::is_any_of("\n"));
    const std::string result[] = {
        "{",
        "   \"metadata\" : {",
        "      \"name\" : \"Mantid Notebook\"",
        "   },",
        "   \"nbformat\" : 3,",
        "   \"nbformat_minor\" : 0,",
        "   \"worksheets\" : [",
        "      {",
        "         \"cells\" : [",
        "            {",
        "               \"cell_type\" : \"markdown\",",
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

  void testTableString() {

    std::map<int, std::set<int>> rows;
    rows[0].insert(0);
    rows[0].insert(1);
    rows[1].insert(0);
    rows[1].insert(1);
    std::string output = tableString(m_model, reflWhitelist(), rows);

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
    for (auto it = notebookLines.begin(); it != notebookLines.end();
         ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
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

  void testReduceRowString() {

    std::map<std::string, std::string> userPreProcessingOptions = {
        {"Run(s)", ""}, {"Transmission Run(s)", ""}};

    boost::tuple<std::string, std::string> output = reduceRowString(
        0, 1, m_instrument, m_model, reflWhitelist(), reflPreprocessMap("TOF_"),
        reflProcessor(), userPreProcessingOptions, "");

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

  void testReduceRowStringNoPreProcessing() {

    std::map<std::string, DataProcessorPreprocessingAlgorithm>
        emptyPreProcessMap;
    std::map<std::string, std::string> emptyPreProcessingOptions;

    boost::tuple<std::string, std::string> output = reduceRowString(
        0, 1, m_instrument, m_model, reflWhitelist(), emptyPreProcessMap,
        reflProcessor(), emptyPreProcessingOptions, "");

    const std::string result[] = {
        "IvsQ_TOF_12346, IvsLam_TOF_12346, _ = "
        "ReflectometryReductionOneAuto(InputWorkspace = "
        "12346, "
        "ThetaIn = "
        "1.5, MomentumTransferMinimum = 1.4, "
        "MomentumTransferMaximum = "
        "2.9, "
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

  void testPostprocessGroupString() {

    std::string userOptions = "Params = '0.1, -0.04, 2.9', StartOverlaps = "
                              "'1.4, 0.1, 1.4', EndOverlaps = '1.6, 2.9, 1.6'";

    DataProcessorWhiteList whitelist;
    whitelist.addElement("Run(s)", "InputWorkspace", "", true);
    whitelist.addElement("Angle", "ThetaIn", "");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun", "");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Group", "Group", "");
    whitelist.addElement("Options", "Options", "");

    std::set<int> groups;
    groups.insert(0);
    groups.insert(1);
    boost::tuple<std::string, std::string> output = postprocessGroupString(
        groups, m_model, whitelist, reflProcessor(),
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
    unprocessed_ws.emplace_back("TEST_WS1_1, TEST_WS1_2");
    unprocessed_ws.emplace_back("TEST_WS2_1, TEST_WS2_2");

    std::vector<std::string> postprocessed_ws;
    postprocessed_ws.emplace_back("TEST_WS3");
    postprocessed_ws.emplace_back("TEST_WS4");

    std::string output = plotsString(
        unprocessed_ws, boost::algorithm::join(postprocessed_ws, "_"),
        reflProcessor());

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
};

#endif // MANTID_MANTIDWIDGETS_DATAPROCESSORGENERATENOTEBOOKTEST_H
