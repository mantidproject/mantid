#ifndef SAVEILLCOSMOSASCIITEST_H_
#define SAVEILLCOSMOSASCIITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveILLCosmosAscii.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveILLCosmosAsciiTest : public CxxTest::TestSuite {

public:
  static SaveILLCosmosAsciiTest *createSuite() {
    return new SaveILLCosmosAsciiTest();
  }
  static void destroySuite(SaveILLCosmosAsciiTest *suite) { delete suite; }

  SaveILLCosmosAsciiTest() {
    m_filename = "SaveILLCosmosAsciiTestFile.txt";
    m_name = "SaveILLCosmosAsciiWS";
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
  ~SaveILLCosmosAsciiTest() override {}

  void testInit() {

  }

  void testExec() {
    // create a new workspace and then delete it later on
    createWS();

    SaveILLCosmosAscii alg;
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveILLCosmosAscii");
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
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT_EQUALS(columns.at(0), "");
    TS_ASSERT_EQUALS(columns.at(1), 1.5);
    TS_ASSERT_EQUALS(columns.at(2), 1);
    TS_ASSERT_EQUALS(columns.at(3), 1);
    TS_ASSERT_EQUALS(columns.at(4), 0.6);
    in.close();

    cleanupafterwards();
  }
  void testNoX() {
    // create a new workspace and then delete it later on
    createWS(true);

    SaveILLCosmosAscii alg;
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveILLCosmosAscii");
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
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT_EQUALS(columns.at(0), "");
    TS_ASSERT_EQUALS(columns.at(1), "0");
    TS_ASSERT_EQUALS(columns.at(2), "1");
    TS_ASSERT_EQUALS(columns.at(3), "1");
    TS_ASSERT((columns.at(4) == "nan") || (columns.at(4) == "inf"));
    in.close();

    cleanupafterwards();
  }
  void testNoY() {
    // create a new workspace and then delete it later on
    createWS(false, true);

    SaveILLCosmosAscii alg;
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveILLCosmosAscii");
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
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT_EQUALS(columns[0], "");
    TS_ASSERT_EQUALS(columns[1], "1.5");
    TS_ASSERT_EQUALS(columns[2], "0.0");
    TS_ASSERT_EQUALS(columns[3], "1.0");
    TS_ASSERT_EQUALS(columns[4], "0.6");
    in.close();

    cleanupafterwards();
  }
  void testNoE() {
    // create a new workspace and then delete it later on
    createWS(false, false, true);

    SaveILLCosmosAscii alg;
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveILLCosmosAscii");
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
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT_EQUALS(columns.at(0), "");
    TS_ASSERT_EQUALS(columns.at(1), "1.5");
    TS_ASSERT_EQUALS(columns.at(2), "1");
    TS_ASSERT_EQUALS(columns.at(3), "0");
    TS_ASSERT_EQUALS(columns.at(4), "0.6");
    in.close();

    cleanupafterwards();
  }
  void testParameters() {
    // create a new workspace and then delete it later on
    createWS(false, false, false, true);

    SaveILLCosmosAscii alg;
    alg->setProperty("InputWorkspace", m_name);
    alg->setProperty("Filename", m_filename);
    alg->setProperty("UserContact", "John Smith");
    alg->setProperty("Title", "Testing this algorithm");
    alg->setProperty("Separator", "comma");
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveILLCosmosAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    headingsTests(in, fullline, true, ",");
    getline(in, fullline);

    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of(","),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 5);
    // the first is black due to the leading separator
    TS_ASSERT_EQUALS(columns.at(0), "");
    TS_ASSERT_EQUALS(columns.at(1), "1.5");
    TS_ASSERT_EQUALS(columns.at(2), "1");
    TS_ASSERT_EQUALS(columns.at(3), "1");
    TS_ASSERT_EQUALS(columns.at(4), "0.6");
    in.close();

    cleanupafterwards();
  }
  void test_fail_invalid_workspace() {
    SaveILLCosmosAscii alg;
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_long_filename = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_ANYTHING(
        alg->setPropertyValue("InputWorkspace", "NotARealWS"));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
    TS_ASSERT(!alg->isExecuted());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(m_long_filename).exists());
  }

private:
  void headingsTests(std::ifstream &in, std::string &fullline,
                     bool propertiesLogs = false, std::string sep = "\t") {
    getline(in, fullline);
    TS_ASSERT_EQUALS(fullline, "MFT");
    getline(in, fullline);
    TS_ASSERT_EQUALS(fullline, "Instrument: ");
    getline(in, fullline);
    if (propertiesLogs) {
      TS_ASSERT_EQUALS(fullline, "User-local contact: John Smith");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Title: Testing this algorithm");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Subtitle: ILL COSMOS save test");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Start date + time: 2011-12-16T01:27:30");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "End date + time: 2011-12-16T02:13:31");
    } else {
      TS_ASSERT_EQUALS(fullline, "User-local contact: ");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Title: ");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Subtitle: ");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "Start date + time: ");
      getline(in, fullline);
      TS_ASSERT_EQUALS(fullline, "End date + time: ");
    }
    getline(in, fullline);
    TS_ASSERT_EQUALS(fullline, "Number of file format: 2");
    getline(in, fullline);
    std::cout << sep;
    TS_ASSERT_EQUALS(fullline, "Number of data points:" + sep + "9");
    getline(in, fullline);
    getline(in, fullline);
    TS_ASSERT_EQUALS(fullline,
              sep + "q" + sep + "refl" + sep + "refl_err" + sep + "q_res");
  }
  void createWS(bool zeroX = false, bool zeroY = false, bool zeroE = false,
                bool createLogs = false) {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(1, 10);

    if (createLogs) {
      ws->mutableRun().addProperty("run_title",
                                   std::string("ILL COSMOS save test"));
      ws->mutableRun().addProperty("run_start",
                                   std::string("2011-12-16T01:27:30"));
      ws->mutableRun().addProperty("run_end",
                                   std::string("2011-12-16T02:13:31"));
    }

    AnalysisDataService::Instance().addOrReplace(m_name, ws);
    // Check if any of X, Y or E should be zeroed to check for divide by zero or
    // similiar
    if (zeroX)
      ws->x(0) = m_data0;
    else
      ws->x(0) = m_dataX;

    if (zeroY)
      ws->y(0) = m_data0;
    else
      ws->y(0) = m_dataY;

    if (zeroE)
      ws->e(0) = m_data0;
    else
      ws->e(0) = m_dataE;
  }
  void cleanupafterwards() {
    Poco::File(m_long_filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }
  std::string m_filename, m_name, m_long_filename;
  std::vector<double> m_dataX, m_dataY, m_dataE, m_data0;
};
#endif /*SAVEILLCOSMOSASCIITEST_H_*/
