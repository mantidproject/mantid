#ifndef MANTID_CUSTOMINTERFACES_REFLGENERATENOTEBOOKTEST_H
#define MANTID_CUSTOMINTERFACES_REFLGENERATENOTEBOOKTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Reflectometry/ReflGenerateNotebook.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflVectorString.h"
#include "MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ITableWorkspace.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

class ReflGenerateNotebookTest : public CxxTest::TestSuite {

private:

  ITableWorkspace_sptr createWorkspace(const std::string &wsName) {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    auto colRuns = ws->addColumn("str", "Run(s)");
    auto colTheta = ws->addColumn("str", "ThetaIn");
    auto colTrans = ws->addColumn("str", "TransRun(s)");
    auto colQmin = ws->addColumn("str", "Qmin");
    auto colQmax = ws->addColumn("str", "Qmax");
    auto colDqq = ws->addColumn("str", "dq/q");
    auto colScale = ws->addColumn("double", "Scale");
    auto colStitch = ws->addColumn("int", "StitchGroup");
    auto colOptions = ws->addColumn("str", "Options");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);
    colStitch->setPlotType(0);
    colOptions->setPlotType(0);

    if (wsName.length() > 0)
      AnalysisDataService::Instance().addOrReplace(wsName, ws);

    return ws;
  }

  ITableWorkspace_sptr createPrefilledWorkspace(const std::string &wsName) {
    auto ws = createWorkspace(wsName);
    TableRow row = ws->appendRow();
    row << "12345"
    << "0.5"
    << ""
    << "0.1"
    << "1.6"
    << "0.04" << 1.0 << 0 << "";
    row = ws->appendRow();
    row << "12346"
    << "1.5"
    << ""
    << "1.4"
    << "2.9"
    << "0.04" << 1.0 << 0 << "";
    row = ws->appendRow();
    row << "24681"
    << "0.5"
    << ""
    << "0.1"
    << "1.6"
    << "0.04" << 1.0 << 1 << "";
    row = ws->appendRow();
    row << "24682"
    << "1.5"
    << ""
    << "1.4"
    << "2.9"
    << "0.04" << 1.0 << 1 << "";
    return ws;
  }

  void createModel(const std::string &wsName)
  {
    ITableWorkspace_sptr prefilled_ws = createPrefilledWorkspace(wsName);
    m_model.reset(new QReflTableModel(prefilled_ws));
  }

  std::string m_wsName;
  std::string m_instrument;
  QReflTableModel_sptr m_model;
  std::set<int> m_rows;
  std::map<int,std::set<int> > m_groups;
  ColNumbers col_nums;

public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflGenerateNotebookTest *createSuite() {
    return new ReflGenerateNotebookTest();
  }
  static void destroySuite(ReflGenerateNotebookTest *suite) { delete suite; }

  ReflGenerateNotebookTest() : col_nums(0, 2, 8, 1, 3, 4, 5, 6, 7) { FrameworkManager::Instance(); }

  // Create a notebook to test
  void setUp()
  {
    m_wsName = "TESTWORKSPACE";
    m_instrument = "INSTRUMENT";

    createModel(m_wsName);

    // Populate rows with every index in the model
    for(int idx = 0; idx < m_model->rowCount(); ++idx)
      m_rows.insert(idx);

    //Map group numbers to the set of rows in that group we want to process
    for(auto it = m_rows.begin(); it != m_rows.end(); ++it)
      m_groups[m_model->data(m_model->index(*it, col_nums.group)).toInt()].insert(*it);
  }

  void testGenerateNotebook()
  {
    std::unique_ptr<ReflGenerateNotebook> notebook(new ReflGenerateNotebook(m_wsName, m_model, m_instrument,
                                                   col_nums.runs, col_nums.transmission, col_nums.options, col_nums.angle,
                                                   col_nums.qmin, col_nums.qmax, col_nums.dqq, col_nums.scale, col_nums.group));

    std::string generatedNotebook = notebook->generateNotebook(m_groups, m_rows);

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
    for (int i=0; i<11; ++i)
    {
      TS_ASSERT_EQUALS(notebookLines[i], result[i])
    }

  }

  void testPlot1DString()
  {
    std::vector<std::string> ws_names;
    ws_names.push_back("workspace1");
    ws_names.push_back("workspace2");

    std::string output = plot1DString(ws_names, "Plot Title");

    const std::string result = "fig = plots([workspace1, workspace2], title=Plot Title, legendLocation=[1, 1, 4])\n";

    TS_ASSERT_EQUALS(output, result)
  }

  void testTableString()
  {
    std::string output = tableString(m_model, col_nums, m_rows);

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    const std::string result[] = {
      "Run(s) | Angle | Transmission Run(s) | Q min | Q max | dQ/Q | Scale | Group | Options",
      "------ | ----- | ------------------- | ----- | ----- | ---- | ----- | ----- | -------",
      "12345 | 0.5 |  | 0.1 | 1.6 | 0.04 | 1 | 0 | ",
      "12346 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 | 0 | ",
      "24681 | 0.5 |  | 0.1 | 1.6 | 0.04 | 1 | 1 | ",
      "24682 | 1.5 |  | 1.4 | 2.9 | 0.04 | 1 | 1 | ",
      ""
    };

    int i=0;
    for (auto it = notebookLines.begin(); it != notebookLines.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, result[i])
    }

  }

  void testVectorString()
  {
    std::vector<std::string> stringVector;
    stringVector.push_back("A");
    stringVector.push_back("B");
    stringVector.push_back("C");

    const std::string stringOutput = vectorString(stringVector);

    std::vector<int> intVector;
    intVector.push_back(1);
    intVector.push_back(2);
    intVector.push_back(3);

    const std::string intOutput = vectorString(intVector);

    // Test string list output is correct for vector of strings and vector of ints
    TS_ASSERT_EQUALS(stringOutput, "A, B, C")
    TS_ASSERT_EQUALS(intOutput, "1, 2, 3")
  }

  void testTitleString()
  {
    // Test with workspace name
    std::string output = titleString("TEST_WORKSPACE");

    const std::string result[] = {
      "Processed data from workspace: TEST_WORKSPACE",
      "---------------",
      "Notebook generated from the ISIS Reflectometry (Polref) Interface",
      ""
    };

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    int i=0;
    for (auto it = notebookLines.begin(); it != notebookLines.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    // Test without workspace name
    std::string outputEmptyStr = titleString("");

    const std::string resultEmptyStr[] = {
      "Processed data",
      "---------------",
      "Notebook generated from the ISIS Reflectometry (Polref) Interface",
      ""
    };

    std::vector<std::string> notebookLinesEmptyStr;
    boost::split(notebookLinesEmptyStr, outputEmptyStr, boost::is_any_of("\n"));

    i=0;
    for (auto it = notebookLinesEmptyStr.begin(); it != notebookLinesEmptyStr.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, resultEmptyStr[i])
    }

  }

  void testStitchGroupString()
  {
    boost::tuple<std::string, std::string> output = stitchGroupString(m_rows, m_instrument, m_model, col_nums);

    const std::string result[] = {
      "#Stitch workspaces",
      "IvsQ_12345_12346_24681_24682, _ = Stitch1DMany(InputWorkspaces = 'IvsQ_12345, IvsQ_12346, IvsQ_24681,"
        " IvsQ_24682', Params = '0.1, -0.04, 2.9', StartOverlaps = '1.4, 0.1, 1.4', EndOverlaps = '1.6, 2.9, 1.6')",
      ""
    };

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, boost::get<0>(output), boost::is_any_of("\n"));

    int i=0;
    for (auto it = notebookLines.begin(); it != notebookLines.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, result[i])
    }

  }

  void testPlotsFunctionString()
  {
    std::string output = plotsFunctionString();

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    const std::string result[] = {
      "#Import some useful tools for plotting",
        "from MantidIPython import *"
    };

    // Check that the lines are output as expected
    TS_ASSERT_EQUALS(notebookLines[0], result[0]);
    TS_ASSERT_EQUALS(notebookLines[1], result[1]);
  }

  void testPlotsString()
  {
    std::vector<std::string> unstitched_ws;
    unstitched_ws.push_back("TEST_WS1");
    unstitched_ws.push_back("TEST_WS2");

    std::vector<std::string> IvsLam_ws;
    IvsLam_ws.push_back("TEST_WS3");
    IvsLam_ws.push_back("TEST_WS4");

    std::string output = plotsString(unstitched_ws, IvsLam_ws, "TEST_WS5");

    const std::string result[] = {
      "#Group workspaces to be plotted on same axes",
      "unstitchedGroupWS = GroupWorkspaces(InputWorkspaces = 'TEST_WS1, TEST_WS2')",
      "IvsLamGroupWS = GroupWorkspaces(InputWorkspaces = 'TEST_WS3, TEST_WS4')",
      "#Plot workspaces",
      "fig = plots([unstitchedGroupWS, TEST_WS5, IvsLamGroupWS], title=['I vs Q Unstitched', 'I vs Q Stitiched', 'I vs Lambda'], legendLocation=[1, 1, 4])",
      ""
    };

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, output, boost::is_any_of("\n"));

    int i=0;
    for (auto it = notebookLines.begin(); it != notebookLines.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, result[i])
    }

  }

  void testReduceRowString()
  {
    boost::tuple<std::string, std::string, std::string> output = reduceRowString(1, m_instrument, m_model, col_nums);

    const std::string result[] = {
      "TOF_12346 = Load(Filename = 'INSTRUMENT12346')",
      "IvsQ_12346, IvsLam_12346, theta_12346 = ReflectometryReductionOneAuto(InputWorkspace = 'TOF_12346', ThetaIn = 1.5)",
      "IvsQ_12346 = Rebin(IvsQ_12346, Params = '1.4, -0.04, 2.9')",
      ""
    };

    std::vector<std::string> notebookLines;
    boost::split(notebookLines, boost::get<0>(output), boost::is_any_of("\n"));

    int i=0;
    for (auto it = notebookLines.begin(); it != notebookLines.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, result[i])
    }

  }

  void testPlusString()
  {
    std::string output = plusString("INPUT_WS", "OUTPUT_WS");
    const std::string result = "OUTPUT_WS = Plus('LHSWorkspace' = OUTPUT_WS, 'RHSWorkspace' = INPUT_WS)\n";
    TS_ASSERT_EQUALS(output, result)
  }

  void testLoadRunString()
  {
    boost::tuple<std::string, std::string> output = loadRunString("12345", m_instrument);
    const std::string result = "TOF_12345 = Load(Filename = 'INSTRUMENT12345')\n";
    TS_ASSERT_EQUALS(boost::get<0>(output), result)
  }

  void testGetRunNumber()
  {
    // Test with no run number in string
    std::string output = getRunNumber("TEST_WORKSPACE");
    const std::string result = "TEST_WORKSPACE";
    TS_ASSERT_EQUALS(output, result)

    // Test with instrument and number
    std::string output1 = getRunNumber("INSTRUMENT12345");
    const std::string result1 = "12345";
    TS_ASSERT_EQUALS(output1, result1)
  }

  void testScaleString()
  {
    boost::tuple<std::string, std::string> output = scaleString("12345", 1.0);
    const std::string result = "IvsQ_12345 = Scale(InputWorkspace = IvsQ_12345, Factor = 1)\n";
    TS_ASSERT_EQUALS(boost::get<0>(output), result)
  }

  void testVectorParamString()
  {
    std::vector<std::string> stringVector;
    stringVector.push_back("A");
    stringVector.push_back("B");
    stringVector.push_back("C");

    const std::string stringOutput = vectorParamString("PARAM_NAME", stringVector);

    TS_ASSERT_EQUALS(stringOutput, "PARAM_NAME = 'A, B, C'")
  }

  void testRebinString()
  {
    boost::tuple<std::string, std::string> output = rebinString(1, "12345", m_model, col_nums);
    const std::string result = "IvsQ_12345 = Rebin(IvsQ_12345, Params = '1.4, -0.04, 2.9')\n";
    TS_ASSERT_EQUALS(boost::get<0>(output), result)
  }

};

#endif //MANTID_CUSTOMINTERFACES_REFLGENERATENOTEBOOKTEST_H
