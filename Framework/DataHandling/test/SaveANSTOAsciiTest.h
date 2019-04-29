// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SAVEANSTOASCIITEST_H_
#define SAVEANSTOASCIITEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/SaveANSTOAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include <Poco/File.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::HistogramData;
using namespace Mantid::DataObjects;

class SaveANSTOAsciiTest : public CxxTest::TestSuite {

public:
  static SaveANSTOAsciiTest *createSuite() { return new SaveANSTOAsciiTest(); }
  static void destroySuite(SaveANSTOAsciiTest *suite) { delete suite; }

  SaveANSTOAsciiTest() {
    m_filename = "SaveANSTOAsciiTestFile.txt";
    m_name = "SaveANSTOAsciiWS";
    for (int i = 1; i < 11; ++i) {
      m_dataX.emplace_back(i);
      m_dataY.emplace_back(i);
      m_dataE.emplace_back(i);
    }
    m_dataX.emplace_back(11);
  }

  void testExec() {
    // create a new workspace and then delete it later on
    createWS();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveANSTOAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveANSTOAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(0)), 1.5, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 1., 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 1., 0.01);
    TS_ASSERT_EQUALS(columns.at(3), "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void testNoX() {
    // create a new workspace and then delete it later on
    createWS(true);

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveANSTOAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveANSTOAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(0)), 0., 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 1., 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 1., 0.01);
    TS_ASSERT_EQUALS(columns.at(3), "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void testNoY() {
    // create a new workspace and then delete it later on
    createWS(false, true);

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveANSTOAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveANSTOAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(0)), 1.5, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 0., 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 1., 0.01);
    TS_ASSERT_EQUALS(columns.at(3), "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void testNoE() {
    // create a new workspace and then delete it later on
    createWS(false, false, true);

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveANSTOAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveANSTOAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of("\t"),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(0)), 1.5, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 1., 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 0., 0.01);
    TS_ASSERT_EQUALS(columns.at(3), "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void testParameters() {
    // create a new workspace and then delete it later on
    createWS();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveANSTOAscii");
    alg->setPropertyValue("InputWorkspace", m_name);
    alg->setPropertyValue("Filename", m_filename);
    alg->setPropertyValue("Separator", "comma");
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    if (!alg->isExecuted()) {
      TS_FAIL("Could not run SaveANSTOAscii");
    }
    m_long_filename = alg->getPropertyValue("Filename");
    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(m_long_filename).exists());
    std::ifstream in(m_long_filename.c_str());
    std::string fullline;
    getline(in, fullline);
    std::vector<std::string> columns;
    boost::split(columns, fullline, boost::is_any_of(","),
                 boost::token_compress_on);
    TS_ASSERT_EQUALS(columns.size(), 4);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(0)), 1.5, 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(1)), 1., 0.01);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(columns.at(2)), 1., 0.01);
    TS_ASSERT_EQUALS(columns.at(3), "0.000000000000000e+00");
    in.close();

    cleanupafterwards();
  }
  void test_fail_invalid_workspace() {
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("SaveANSTOAscii");
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
  void createWS(bool zeroX = false, bool zeroY = false, bool zeroE = false) {
    // Check if any of X, Y or E should be zeroed to check for divide by zero or
    // similiar
    BinEdges edges = zeroX ? BinEdges(11, 0.) : BinEdges(m_dataX);
    Counts counts = zeroY ? Counts(10, 0.) : Counts(m_dataY);
    CountStandardDeviations stddev = zeroE ? CountStandardDeviations(10, 0.)
                                           : CountStandardDeviations(m_dataE);
    MatrixWorkspace_sptr ws =
        create<Workspace2D>(1, Histogram(edges, counts, stddev));
    AnalysisDataService::Instance().addOrReplace(m_name, ws);
  }
  void cleanupafterwards() {
    Poco::File(m_long_filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }
  std::string m_filename, m_name, m_long_filename;
  std::vector<double> m_dataX, m_dataY, m_dataE;
};

#endif /*SAVEANSTOTEST_H_*/
