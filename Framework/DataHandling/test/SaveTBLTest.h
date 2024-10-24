// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SaveTBL.h"
#include "MantidDataObjects/Workspace2D.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveTBLTest : public CxxTest::TestSuite {

public:
  static SaveTBLTest *createSuite() { return new SaveTBLTest(); }
  static void destroySuite(SaveTBLTest *suite) { delete suite; }

  SaveTBLTest() : m_name("SaveTBLTestWS"), m_filename("SaveTBLTest.tbl"), m_abspath() {}

  ~SaveTBLTest() override = default;

  void testNoQuotes() {
    ITableWorkspace_sptr ws = CreatePopulatedWorkspace();

    auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveTBL");
    }

    TS_ASSERT(Poco::File(m_abspath).exists());
    std::ifstream file(m_abspath.c_str());
    std::string line = "";
    getline(file, line);
    TS_ASSERT_EQUALS(line, "Run(s),ThetaIn,TransRun(s),Qmin,Qmax,dq/q,Scale,StitchGroup,Options");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13460,0.7,13463,0.01,0.06,0.04,2.0,1,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13462,2.3,13463,0.035,0.3,0.04,2.0,1,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13470,2.3,13463,0.035,0.3,0.04,2.0,1,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13460,0.7,13463,0.01,0.06,0.04,2.0,2,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13462,2.3,13463,0.035,0.3,0.04,2.0,2,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13470,2.3,13463,0.035,0.3,0.04,2.0,3,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13460,0.7,13463,0.01,0.06,0.04,2.0,0,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13462,2.3,13463,0.035,0.3,0.4,3.0,3,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13470,2.3,13463,0.035,0.3,0.04,2.0,4,");

    file.close();
    cleanupafterwards();
  }

  void testQuotes() {
    ITableWorkspace_sptr ws = CreatePopulatedWorkspace();

    TableRow row = ws->appendRow();
    row << "13460"
        << "0.7"
        << "13463+13464"
        << "0.01"
        << "0.06"
        << "0.04"
        << "2.0"
        << "4"
        << "";

    row = ws->appendRow();
    row << "13470"
        << "2.3"
        << "13463+13464"
        << "0.035"
        << "0.3"
        << "0.04"
        << "2.0"
        << "5"
        << "";

    auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveTBL");
    }

    TS_ASSERT(Poco::File(m_abspath).exists());
    std::ifstream file(m_abspath.c_str());
    std::string line = "";
    getline(file, line);
    TS_ASSERT_EQUALS(line, "Run(s),ThetaIn,TransRun(s),Qmin,Qmax,dq/q,Scale,StitchGroup,Options");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13460,0.7,13463,0.01,0.06,0.04,2.0,1,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13462,2.3,13463,0.035,0.3,0.04,2.0,1,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13470,2.3,13463,0.035,0.3,0.04,2.0,1,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13460,0.7,13463,0.01,0.06,0.04,2.0,2,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13462,2.3,13463,0.035,0.3,0.04,2.0,2,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13470,2.3,13463,0.035,0.3,0.04,2.0,3,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13460,0.7,13463,0.01,0.06,0.04,2.0,0,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13462,2.3,13463,0.035,0.3,0.4,3.0,3,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13470,2.3,13463,0.035,0.3,0.04,2.0,4,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13460,0.7,13463+13464,0.01,0.06,0.04,2.0,4,");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13470,2.3,13463+13464,0.035,0.3,0.04,2.0,5,");
    file.close();
    cleanupafterwards();
  }

  void testWithExtraColumns() {
    auto ws = CreateWorkspace();
    auto extraValues = ws->addColumn("str", "ExtraValues");
    extraValues->setPlotType(0);

    TableRow row = ws->appendRow();
    row << "13460"
        << "0.7"
        << "13463+13464"
        << "0.01"
        << "0.06"
        << "0.04"
        << "2.0"
        << "4"
        << ""
        << "Some Value";

    row = ws->appendRow();
    row << "13470"
        << "2.3"
        << "13463+13464"
        << "0.035"
        << "0.3"
        << "0.04"
        << "2.0"
        << "5"
        << ""
        << "Some Other Value";

    auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveTBL");
    }

    TS_ASSERT(Poco::File(m_abspath).exists());
    std::ifstream file(m_abspath.c_str());
    std::string line = "";
    getline(file, line);
    TS_ASSERT_EQUALS(line, "Run(s),ThetaIn,TransRun(s),Qmin,Qmax,dq/"
                           "q,Scale,StitchGroup,Options,ExtraValues");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13460,0.7,13463+13464,0.01,0.06,0.04,2.0,4,,Some Value");
    getline(file, line);
    TS_ASSERT_EQUALS(line, "13470,2.3,13463+13464,0.035,0.3,0.04,2.0,5,,Some Other Value");
    file.close();
    cleanupafterwards();
  }

  void testGroupPass() {
    ITableWorkspace_sptr ws = CreatePopulatedWorkspace();
    TableRow row = ws->appendRow();
    row << "13460"
        << "0.7"
        << "13463"
        << "0.01"
        << "0.06"
        << "0.04"
        << "2.0"
        << "1"
        << "";
    row = ws->appendRow();
    row << "13464"
        << "0.73"
        << "13463"
        << "0.012"
        << "0.064"
        << "0.04"
        << "2.0"
        << "1"
        << "";
    auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(Poco::File(m_abspath).exists());
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_name));
  }

  void testIntegerGroupColumn() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(m_name, ws);
    auto colRuns = ws->addColumn("str", "Run(s)");
    auto colTheta = ws->addColumn("str", "ThetaIn");
    auto colTrans = ws->addColumn("str", "TransRun(s)");
    auto colQmin = ws->addColumn("str", "Qmin");
    auto colQmax = ws->addColumn("str", "Qmax");
    auto colDqq = ws->addColumn("str", "dq/q");
    auto colScale = ws->addColumn("str", "Scale");
    auto colGroup = ws->addColumn("int", "Group");
    auto colOptions = ws->addColumn("str", "Options");
    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);

    TableRow row = ws->appendRow();
    row << "13460"
        << "0.7"
        << "13463"
        << "0.01"
        << "0.06"
        << "0.04"
        << "2.0" << 1 << "";

    auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(m_abspath).exists());
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_name));
  }

  void testLoadWithLoadTBL() {
    ITableWorkspace_sptr ws = CreatePopulatedWorkspace();

    auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveTBL");
    }

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_name));
    auto algLoad = Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    algLoad->setRethrows(true);
    algLoad->setPropertyValue("OutputWorkspace", m_name);
    algLoad->setPropertyValue("Filename", m_abspath);
    TS_ASSERT_THROWS_NOTHING(algLoad->execute());
    if (!alg->isExecuted()) {
      TS_FAIL("Could not run LoadTBL");
    }

    cleanupafterwards();
  }

private:
  void cleanupafterwards() {
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_name));
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  ITableWorkspace_sptr CreateWorkspace() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(m_name, ws);
    auto colRuns = ws->addColumn("str", "Run(s)");
    auto colTheta = ws->addColumn("str", "ThetaIn");
    auto colTrans = ws->addColumn("str", "TransRun(s)");
    auto colQmin = ws->addColumn("str", "Qmin");
    auto colQmax = ws->addColumn("str", "Qmax");
    auto colDqq = ws->addColumn("str", "dq/q");
    auto colScale = ws->addColumn("str", "Scale");
    auto colStitch = ws->addColumn("str", "StitchGroup");
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

    return ws;
  }

  ITableWorkspace_sptr CreatePopulatedWorkspace() {
    auto ws = CreateWorkspace();
    TableRow row = ws->appendRow();
    row << "13460"
        << "0.7"
        << "13463"
        << "0.01"
        << "0.06"
        << "0.04"
        << "2.0"
        << "1"
        << "";

    row = ws->appendRow();
    row << "13462"
        << "2.3"
        << "13463"
        << "0.035"
        << "0.3"
        << "0.04"
        << "2.0"
        << "1"
        << "";

    row = ws->appendRow();
    row << "13470"
        << "2.3"
        << "13463"
        << "0.035"
        << "0.3"
        << "0.04"
        << "2.0"
        << "1"
        << "";

    row = ws->appendRow();
    row << "13460"
        << "0.7"
        << "13463"
        << "0.01"
        << "0.06"
        << "0.04"
        << "2.0"
        << "2"
        << "";

    row = ws->appendRow();
    row << "13462"
        << "2.3"
        << "13463"
        << "0.035"
        << "0.3"
        << "0.04"
        << "2.0"
        << "2"
        << "";

    row = ws->appendRow();
    row << "13470"
        << "2.3"
        << "13463"
        << "0.035"
        << "0.3"
        << "0.04"
        << "2.0"
        << "3"
        << "";

    row = ws->appendRow();
    row << "13460"
        << "0.7"
        << "13463"
        << "0.01"
        << "0.06"
        << "0.04"
        << "2.0"
        << "0"
        << "";

    row = ws->appendRow();
    row << "13462"
        << "2.3"
        << "13463"
        << "0.035"
        << "0.3"
        << "0.4"
        << "3.0"
        << "3"
        << "";

    row = ws->appendRow();
    row << "13470"
        << "2.3"
        << "13463"
        << "0.035"
        << "0.3"
        << "0.04"
        << "2.0"
        << "4"
        << "";

    return ws;
  }

  std::string m_name;
  std::string m_filename;
  std::string m_abspath;
};
