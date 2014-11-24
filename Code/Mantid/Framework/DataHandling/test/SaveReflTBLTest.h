#ifndef SAVEREFLTBLTEST_H_
#define SAVEREFLTBLTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveReflTBL.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TableRow.h"
#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveReflTBLTest : public CxxTest::TestSuite
{

public:

  static SaveReflTBLTest *createSuite() { return new SaveReflTBLTest(); }
  static void destroySuite(SaveReflTBLTest *suite) { delete suite; }

  SaveReflTBLTest(): m_name("SaveReflTBLTestWS"), m_filename("SaveReflTBLTest.tbl"), m_abspath()
  {
  }

  ~SaveReflTBLTest()
  {
  }

  void testNoQuotes()
  {
    ITableWorkspace_sptr ws = CreateWorkspace();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveReflTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if ( ! alg->isExecuted() )
    {
      TS_FAIL("Could not run SaveReflTBL");
    }

    TS_ASSERT( Poco::File(m_abspath).exists() );
    std::ifstream file(m_abspath.c_str());
    std::string line = "";
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,13470,2.3,13463,0.035,0.3,0.04,2");
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,,,,,,0.04,2");
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13470,2.3,13463,0.035,0.3,13462,2.3,13463,0.035,0.3,,,,,,0.04,2");
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13470,2.3,13463,0.035,0.3,,,,,,,,,,,0.04,2");
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13460,0.7,13463,0.01,0.06,,,,,,,,,,,0.04,2");

    file.close();
    cleanupafterwards();
  }

  void testQuotes()
  {
    ITableWorkspace_sptr ws = CreateWorkspace();

    TableRow row = ws->appendRow();
    row << "13460" << "0.7" << "13463,13464" << "0.01" << "0.06" << "0.04" << 2.0 << 4;

    row = ws->appendRow();
    row << "13470" << "2.3" << "13463,13464" << "0.035" << "0.3" << "0.04" << 2.0 << 5;

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveReflTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if ( ! alg->isExecuted() )
    {
      TS_FAIL("Could not run SaveReflTBL");
    }

    TS_ASSERT( Poco::File(m_abspath).exists() );
    std::ifstream file(m_abspath.c_str());
    std::string line = "";
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,13470,2.3,13463,0.035,0.3,0.04,2");
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,,,,,,0.04,2");
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13470,2.3,13463,0.035,0.3,13462,2.3,13463,0.035,0.3,,,,,,0.04,2");
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13470,2.3,13463,0.035,0.3,13460,0.7,\"13463,13464\",0.01,0.06,,,,,,0.04,2");
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13470,2.3,\"13463,13464\",0.035,0.3,,,,,,,,,,,0.04,2");
    getline(file,line);
    TS_ASSERT_EQUALS(line,"13460,0.7,13463,0.01,0.06,,,,,,,,,,,0.04,2");

    file.close();
    cleanupafterwards();
  }

  void testFourGroupFail()
  {
    ITableWorkspace_sptr ws = CreateWorkspace();

    TableRow row = ws->appendRow();
    row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << 2.0 << 1;

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveReflTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(m_abspath).exists() );
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_name));
  }

  void testNotEnoughColumns()
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(m_name, ws);
    auto colRuns = ws->addColumn("str","Run(s)");
    auto colTheta = ws->addColumn("str","ThetaIn");
    auto colTrans = ws->addColumn("str","TransRun(s)");
    auto colQmin = ws->addColumn("str","Qmin");
    auto colQmax = ws->addColumn("str","Qmax");
    auto colDqq = ws->addColumn("str","dq/q");
    auto colScale = ws->addColumn("double","Scale");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);

    TableRow row = ws->appendRow();
    row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << 2.0;

    row = ws->appendRow();
    row << "13462" << "2.3" << "13463" << "0.035" << "0.3" << "0.04" << 2.0;

    row = ws->appendRow();
    row << "13470" << "2.3" << "13463" << "0.035" << "0.3" << "0.04" << 2.0;

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveReflTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(m_abspath).exists() );
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_name));
  }

  void testLoadWithLoadReflTBL()
  {
    ITableWorkspace_sptr ws = CreateWorkspace();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SaveReflTBL");
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    m_abspath = alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    if ( ! alg->isExecuted() )
    {
      TS_FAIL("Could not run SaveReflTBL");
    }

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_name));

    Mantid::API::IAlgorithm_sptr algLoad = Mantid::API::AlgorithmManager::Instance().create("LoadReflTBL");
    algLoad->setRethrows(true);
    algLoad->setPropertyValue("OutputWorkspace", m_name);
    algLoad->setPropertyValue("Filename", m_abspath);
    TS_ASSERT_THROWS_NOTHING(algLoad->execute());
    if ( ! alg->isExecuted() )
    {
      TS_FAIL("Could not run LoadReflTBL");
    }

    cleanupafterwards();
  }

private:

  void cleanupafterwards()
  {
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_name));
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  ITableWorkspace_sptr CreateWorkspace()
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(m_name, ws);
    auto colRuns = ws->addColumn("str","Run(s)");
    auto colTheta = ws->addColumn("str","ThetaIn");
    auto colTrans = ws->addColumn("str","TransRun(s)");
    auto colQmin = ws->addColumn("str","Qmin");
    auto colQmax = ws->addColumn("str","Qmax");
    auto colDqq = ws->addColumn("str","dq/q");
    auto colScale = ws->addColumn("double","Scale");
    auto colStitch = ws->addColumn("int","StitchGroup");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);
    colStitch->setPlotType(0);

    TableRow row = ws->appendRow();
    row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << 2.0 << 1;

    row = ws->appendRow();
    row << "13462" << "2.3" << "13463" << "0.035" << "0.3" << "0.04" << 2.0 << 1;

    row = ws->appendRow();
    row << "13470" << "2.3" << "13463" << "0.035" << "0.3" << "0.04" << 2.0 << 1;

    row = ws->appendRow();
    row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << 2.0 << 2;

    row = ws->appendRow();
    row << "13462" << "2.3" << "13463" << "0.035" << "0.3" << "0.04" << 2.0 << 2;

    row = ws->appendRow();
    row << "13470" << "2.3" << "13463" << "0.035" << "0.3" << "0.04" << 2.0 << 3;

    row = ws->appendRow();
    row << "13460" << "0.7" << "13463" << "0.01" << "0.06" << "0.04" << 2.0 << 0;

    //this row's last two cells will show in the tableworkspace, but the first row in stich group 3's will take priority when saving
    row = ws->appendRow();
    row << "13462" << "2.3" << "13463" << "0.035" << "0.3" << "0.4" << 3.0 << 3;

    row = ws->appendRow();
    row << "13470" << "2.3" << "13463" << "0.035" << "0.3" << "0.04" << 2.0 << 4;

    return ws;
  }

  std::string m_name;
  std::string m_filename;
  std::string m_abspath;
};
#endif /*SAVEREFLTBLTEST_H_*/
