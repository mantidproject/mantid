#ifndef SAVEREFLCUSTOMASCIITEST_H_
#define SAVEREFLCUSTOMASCIITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveReflCustomAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveReflCustomAsciiTest : public CxxTest::TestSuite {

public:
  static SaveReflCustomAsciiTest *createSuite() {
    return new SaveReflCustomAsciiTest();
  }
  static void destroySuite(SaveReflCustomAsciiTest *suite) { delete suite; }

  SaveReflCustomAsciiTest() {
    m_filename = "SaveReflCustomAsciiFile.txt";
    m_name = "SaveReflCustomAsciiWS";
    for (int i = 1; i < 11; ++i) {
      // X, Y and E get [1,2,3,4,5,6,7,8,9,10]
      // 0 gets [0,0,0,0,0,0,0,0,0,0] and is used to make sure there is no
      // problem with divide by zero
      m_dataX.push_back(i);
      m_dataY.push_back(i);
      m_dataE.push_back(i);
      m_data0.push_back(0);
    }
  }
  ~SaveReflCustomAsciiTest() {}

  void testExec() {
    // create a new workspace and then delete it later on
    createWS();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveReflCustomAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveReflCustomAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline);
    getline(in, fullline);

    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    // the first is black due to the leading tab
    TS_ASSERT(columns.at(0) == "");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 2.5, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 2, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(3)), 2, 0.01);
    in.close();

    cleanupafterwards();
  }
  void testNoX() {
    // create a new workspace and then delete it later on
    createWS(true);

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveReflCustomAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveReflCustomAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline);
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    // the first is black due to the leading tab
    TS_ASSERT(columns.at(0) == "");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 0, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 2, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(3)), 2, 0.01);
    in.close();

    cleanupafterwards();
  }
  void testNoY() {
    // create a new workspace and then delete it later on
    createWS(false, true);

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveReflCustomAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveReflCustomAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline);
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    // the first is black due to the leading tab
    TS_ASSERT(columns.at(0) == "");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 2.5, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 0, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(3)), 2, 0.01);
    in.close();

    cleanupafterwards();
  }
  void testNoE() {
    // create a new workspace and then delete it later on
    createWS(false, false, true);

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveReflCustomAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveReflCustomAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline);
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    // the first is black due to the leading tab
    TS_ASSERT(columns.at(0) == "");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 2.5, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 2, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(3)), 0, 0.01);
    in.close();

    cleanupafterwards();
  }
  void testParameters() {
    // create a new workspace and then delete it later on
    createWS(false, false, false, true);

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveReflCustomAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    alg->setPropertyValue("Title", "Testing this algorithm");
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveReflCustomAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline, true);
    getline(in, fullline);

    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    // the first is black due to the leading tab
    TS_ASSERT(columns.at(0) == "");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 1.5, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 1, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(3)), 1, 0.01);
    in.close();

    cleanupafterwards();
  }
  void test_fail_invalid_workspace() {
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveReflCustomAscii");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_long_filename = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_ANYTHING(
        alg->setPropertyValue("InputWorkspace", "NotARealWS"));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(m_long_filename).exists());
  }

private:
  void headingsTests(std::ifstream &in, std::string &fullline,
                     bool propertiesLogs = false) {
    getline(in, fullline);
    getline(in, fullline);
    propertiesLogs = false;
  }
  void createWS(bool zeroX = false, bool zeroY = false, bool zeroE = false,
                bool createLogs = false) {
    createLogs = false;
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(1, 10);
    AnalysisDataService::Instance().addOrReplace(m_name, ws);
    // Check if any of X, Y or E should be zeroed to check for divide by zero or
    // similiar
    if (zeroX) {
      ws->dataX(0) = m_data0;
    } else {
      ws->dataX(0) = m_dataX;
    }

    if (zeroY) {
      ws->dataY(0) = m_data0;
    } else {
      ws->dataY(0) = m_dataY;
    }

    if (zeroE) {
      ws->dataE(0) = m_data0;
    } else {
      ws->dataE(0) = m_dataE;
    }
  }
  void cleanupafterwards() {
    Poco::File(m_long_filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }
  std::string m_filename, m_name, m_long_filename;
  std::vector<double> m_dataX, m_dataY, m_dataE, m_data0;
};
#endif /*SAVEREFLCUSTOMASCIITEST_H_*/
