#ifndef LOADREFLTBLTEST_H_
#define LOADREFLTBLTEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/LoadReflTBL.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TableRow.h"
#include <Poco/File.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class LoadReflTBLTest : public CxxTest::TestSuite
{
public:
  static LoadReflTBLTest *createSuite() { return new LoadReflTBLTest(); }
  static void destroySuite(LoadReflTBLTest *suite) { delete suite; }

  LoadReflTBLTest(): m_filename("LoadReflTBLTest.tbl"), m_wsName("LoadReflTBLTestWS"), m_abspath()
  {
  }

  ~LoadReflTBLTest()
  {
  }

  void testFileNoQuotes()
  {
    //create a file with each line containing different but valid data format
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,13463,0.01,0.06,,,,,,,,,,,0.04,2" << std::endl;
    file << "13469,0.7,13463,0.01,0.06,13470,2.3,13463,0.035,0.3,,,,,,0.04,2" << std::endl;
    file << "13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,13470,2.3,13463,0.035,0.3,0.04,2" << std::endl;
    file << "13460,0.7,13463,0.01,0.06,,,,,,13470,2.3,13463,0.035,0.3,0.04,2" << std::endl;
    file << ",,,,,13470,2.3,13463,0.035,0.3,,,,,,0.04,2" << std::endl;
    file << ",,,,,,,,,,13462,2.3,13463,0.035,0.3,0.04,2" << std::endl;
    file.close();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadReflTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    TS_ASSERT(alg->isExecuted());

    TS_ASSERT_EQUALS(AnalysisDataService::Instance().doesExist(m_wsName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(m_wsName));
    TableWorkspace_sptr outputWS = boost::dynamic_pointer_cast<TableWorkspace>(output);
    TS_ASSERT_EQUALS(outputWS->columnCount(),9);
    TS_ASSERT_EQUALS(outputWS->rowCount(),10);

    //test the first three rows, equivalent to the first two rows of the file.
    TableRow row = outputWS->getRow(0);
    TS_ASSERT_EQUALS(row.cell<std::string>(0),"13460");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(1)),0.7,0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(2),"13463");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(3)),0.01,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)),0.06,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)),0.04,0.001);
    TS_ASSERT_DELTA(row.cell<double>(6),2,0.01);
    TS_ASSERT_EQUALS(row.cell<int>(7),1);

    row = outputWS->getRow(1);
    TS_ASSERT_EQUALS(row.cell<std::string>(0),"13469");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(1)),0.7,0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(2),"13463");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(3)),0.01,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)),0.06,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)),0.04,0.001);
    TS_ASSERT_DELTA(row.cell<double>(6),2,0.01);
    TS_ASSERT_EQUALS(row.cell<int>(7),2);

    row = outputWS->getRow(2);
    TS_ASSERT_EQUALS(row.cell<std::string>(0),"13470");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(1)),2.3,0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(2),"13463");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(3)),0.035,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)),0.3,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)),0.04,0.001);
    TS_ASSERT_DELTA(row.cell<double>(6),2,0.01);
    TS_ASSERT_EQUALS(row.cell<int>(7),2);

    cleanupafterwards();
  }

  void testQuotedFile()
  {
    //create a file with each line containing different but valid data format
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,\"13463,13464\",0.01,0.06,,,,,,,,,,,0.04,2" << std::endl;
    file << "13469,0.7,\"13463,13464\",0.01,0.06,13470,2.3,\"13463,13464\",0.035,0.3,,,,,,0.04,2" << std::endl;
    file << "13460,0.7,\"13463,13464\",0.01,0.06,13462,2.3,\"13463,13464\",0.035,0.3,13470,2.3,\"13463,13464\",0.035,0.3,0.04,2" << std::endl;
    file << "13460,0.7,\"13463,13464\",0.01,0.06,,,,,,13470,2.3,\"13463,13464\",0.035,0.3,0.04,2" << std::endl;
    file << ",,,,,13470,2.3,\"13463,13464\",0.035,0.3,,,,,,0.04,2" << std::endl;
    file << ",,,,,,,,,,13462,2.3,\"13463,13464\",0.035,0.3,0.04,2" << std::endl;
    file.close();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadReflTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    TS_ASSERT(alg->isExecuted());

    TS_ASSERT_EQUALS(AnalysisDataService::Instance().doesExist(m_wsName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(m_wsName));
    TableWorkspace_sptr outputWS = boost::dynamic_pointer_cast<TableWorkspace>(output);
    TS_ASSERT_EQUALS(outputWS->columnCount(),9);
    TS_ASSERT_EQUALS(outputWS->rowCount(),10);

    //test the first three rows, equivalent to the first two rows of the file.
    TableRow row = outputWS->getRow(0);
    TS_ASSERT_EQUALS(row.cell<std::string>(0),"13460");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(1)),0.7,0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(2),"13463,13464");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(3)),0.01,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)),0.06,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)),0.04,0.001);
    TS_ASSERT_DELTA(row.cell<double>(6),2,0.01);
    TS_ASSERT_EQUALS(row.cell<int>(7),1);

    row = outputWS->getRow(1);
    TS_ASSERT_EQUALS(row.cell<std::string>(0),"13469");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(1)),0.7,0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(2),"13463,13464");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(3)),0.01,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)),0.06,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)),0.04,0.001);
    TS_ASSERT_DELTA(row.cell<double>(6),2,0.01);
    TS_ASSERT_EQUALS(row.cell<int>(7),2);

    row = outputWS->getRow(2);
    TS_ASSERT_EQUALS(row.cell<std::string>(0),"13470");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(1)),2.3,0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(2),"13463,13464");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(3)),0.035,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)),0.3,0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)),0.04,0.001);
    TS_ASSERT_DELTA(row.cell<double>(6),2,0.01);
    TS_ASSERT_EQUALS(row.cell<int>(7),2);

    cleanupafterwards();
  }

  void testFewColumns()
  {
    //create a file with each line containing too few columns
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,\"13463,13464\",0.01,0.06,,,,,,,0.04,2" << std::endl;
    file << "13469,0.7,\"13463,13464\",0.01,0.06,13470,2.3,\"13463,13464\",0.035,0.3,,0.04,2" << std::endl;
    file << "13460,0.7,\"13463,13464\",,\"13463,13464\",,13470,2.3,\"13463,13464\",0.035,0.04,2" << std::endl;
    file << "13460,0.7,\"13463,13464\",0.01,0.06,,13470,2.3,\"13463,13464\",0.035,0.3,0.04,2" << std::endl;
    file << "13470,2.3,\"13463,13464\",0.035,0.3,,0.04,2" << std::endl;
    file << ",,,,13462,2.3,\"13463,13464\",0.035,0.3,0.04,2" << std::endl;
    file.close();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadReflTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath= alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void testManyColumns()
  {
    //create a file with each line containing too many columns
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,13463,0.01,0.06,,,,,,,,,,,0.04,2,,,,0.04,2" << std::endl;
    file << "13469,0.7,13463,0.01,0.06,13470,2.3,13463,0.035,0.3,,,,,,0.04,2,,,,0.04,2" << std::endl;
    file << "13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,13470,2.3,13463,0.035,0.3,0.04,2,,,,0.04,2" << std::endl;
    file << "13460,0.7,13463,0.01,0.06,,,,,,13470,2.3,13463,0.035,0.3,0.04,2,,,,0.04,2" << std::endl;
    file << ",,,,,13470,2.3,13463,0.035,0.3,,,,,,0.04,2,,,,0.04,2" << std::endl;
    file << ",,,,,,,,,13462,2.3,13463,0.035,0.3,0.04,2,,,,0.04,2" << std::endl;
    file.close();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadReflTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath= alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void testManyColumnsTwo()
  {
    //create a file with each line containing too many columns
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,\"13463,0.01\",0.06,,,,,,,,,,,0.04,2,,,,0.04,2" << std::endl;
    file << "13469,0.7,13463,\"0.01,0.06\",13470,2.3,13463,0.06,\"13470,0.06,13470\",2.3,13463,0.035,0.3,,,,,,,,,0.04,2,,,,0.04,2" << std::endl;
    file << "13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,13470,2.3,13463,0.035,0.3,0.04,2,,,,0.04,2" << std::endl;
    file << "13460,0.7,\"13463,0.01\",0.06,,,,,,,,,,13470,2.3,\"13463,0.035\",0.3,0.04,2,,,,0.04,2" << std::endl;
    file << ",,,,,13470,2.3,\"13463,0.035\",0.3,,,,,,,,,0.04,2,,,,0.04,2" << std::endl;
    file << ",,,,,,,,,,,,13462,2.3,\"13463,0.035\",0.3,0.04,2,,,,0.04,2" << std::endl;
    file.close();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadReflTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath= alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }
  
  void testBlankFile()
  {
    std::ofstream file(m_filename.c_str());
    file.close();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadReflTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    TS_ASSERT(alg->isExecuted());

    TS_ASSERT_EQUALS(AnalysisDataService::Instance().doesExist(m_wsName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(m_wsName));
    TableWorkspace_sptr outputWS = boost::dynamic_pointer_cast<TableWorkspace>(output);

    //the columns should be there, but no rows
    TS_ASSERT_EQUALS(outputWS->columnCount(),9);
    TS_ASSERT_EQUALS(outputWS->rowCount(),0);

    cleanupafterwards();
  }
  
  void testNoDataFile()
  {
    //create a file with content, and the right amount of delimiters, but no valid data
    std::ofstream file(m_filename.c_str());
    file << ",,,,,,,,,,,,,,,," << std::endl;
    file << ",,,,,,,,,,,,,,,," << std::endl;
    file << ",,,,,,,,,,,,,,,," << std::endl;
    file << ",,,,,,,,,,,,,,,," << std::endl;
    file << ",,,,,,,,,,,,,,,," << std::endl;
    file << ",,,,,,,,,,,,,,,," << std::endl;
    file << ",,,,,,,,,,,,,,,," << std::endl;
    file.close();

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadReflTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    TS_ASSERT(alg->isExecuted());

    TS_ASSERT_EQUALS(AnalysisDataService::Instance().doesExist(m_wsName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(m_wsName));
    TableWorkspace_sptr outputWS = boost::dynamic_pointer_cast<TableWorkspace>(output);

    //the columns should be there, but no rows
    TS_ASSERT_EQUALS(outputWS->columnCount(),9);
    TS_ASSERT_EQUALS(outputWS->rowCount(),0);

    cleanupafterwards();
  }

private:
  const std::string m_filename;
  const std::string m_wsName;
  std::string m_abspath;
  void cleanupafterwards()
  {
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_wsName));
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }
};


#endif //LOADREFLTBLTEST_H_
