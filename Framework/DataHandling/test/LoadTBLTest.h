// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADTBLTEST_H_
#define LOADTBLTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadTBL.h"
#include "cxxtest/TestSuite.h"
#include <Poco/File.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class LoadTBLTest : public CxxTest::TestSuite {
public:
  static LoadTBLTest *createSuite() { return new LoadTBLTest(); }
  static void destroySuite(LoadTBLTest *suite) { delete suite; }

  LoadTBLTest()
      : m_filename("LoadTBLTest.tbl"), m_wsName("LoadTBLTestWS"), m_abspath() {}

  ~LoadTBLTest() override {}

  void testFileNoQuotes() {
    // create a file with each line containing different but valid data format
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,13463,0.01,0.06,,,,,,,,,,,0.04,2\n";
    file << "13469,0.7,13463,0.01,0.06,13470,2.3,13463,0.035,0.3,,,,,,0.04,2\n";
    file << "13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,13470,2.3,"
            "13463,0.035,0.3,0.04,2\n";
    file << "13460,0.7,13463,0.01,0.06,,,,,,13470,2.3,13463,0.035,0.3,0.04,2\n";
    file << ",,,,,13470,2.3,13463,0.035,0.3,,,,,,0.04,2\n";
    file << ",,,,,,,,,,13462,2.3,13463,0.035,0.3,0.04,2\n";
    file.close();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    TS_ASSERT(alg->isExecuted());

    TS_ASSERT_EQUALS(AnalysisDataService::Instance().doesExist(m_wsName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(m_wsName));
    TableWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<TableWorkspace>(output);
    TS_ASSERT_EQUALS(outputWS->columnCount(), 10);
    TS_ASSERT_EQUALS(outputWS->rowCount(), 10);

    // test the first three rows, equivalent to the first two rows of the file.
    TableRow row = outputWS->getRow(0);
    TS_ASSERT_EQUALS(row.cell<std::string>(1), "13460");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(2)), 0.7,
                    0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(3), "13463");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)), 0.01,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)), 0.06,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(6)), 0.04,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(7)), 2,
                    0.01);
    TS_ASSERT_EQUALS(boost::lexical_cast<double>(row.cell<std::string>(0)), 1);

    row = outputWS->getRow(1);
    TS_ASSERT_EQUALS(row.cell<std::string>(1), "13469");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(2)), 0.7,
                    0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(3), "13463");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)), 0.01,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)), 0.06,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(6)), 0.04,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(7)), 2,
                    0.01);
    TS_ASSERT_EQUALS(boost::lexical_cast<double>(row.cell<std::string>(0)), 2);

    row = outputWS->getRow(2);
    TS_ASSERT_EQUALS(row.cell<std::string>(1), "13470");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(2)), 2.3,
                    0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(3), "13463");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)),
                    0.035, 0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)), 0.3,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(6)), 0.04,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(7)), 2,
                    0.01);
    TS_ASSERT_EQUALS(boost::lexical_cast<double>(row.cell<std::string>(0)), 2);

    TS_ASSERT(outputWS->getColumn("HiddenOptions") != nullptr);
    cleanupafterwards();
  }

  void testQuotedFile() {
    // create a file with each line containing different but valid data format
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,\"13463,13464\",0.01,0.06,,,,,,,,,,,0.04,2\n";
    file << "13469,0.7,\"13463,13464\",0.01,0.06,13470,2.3,\"13463,13464\",0."
            "035,0.3,,,,,,0.04,2\n";
    file << "13460,0.7,\"13463,13464\",0.01,0.06,13462,2.3,\"13463,13464\",0."
            "035,0.3,13470,2.3,\"13463,13464\",0.035,0.3,0.04,2\n";
    file << "13460,0.7,\"13463,13464\",0.01,0.06,,,,,,13470,2.3,\"13463,"
            "13464\",0.035,0.3,0.04,2\n";
    file << ",,,,,13470,2.3,\"13463,13464\",0.035,0.3,,,,,,0.04,2\n";
    file << ",,,,,,,,,,13462,2.3,\"13463,13464\",0.035,0.3,0.04,2\n";
    file.close();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    TS_ASSERT(alg->isExecuted());

    TS_ASSERT_EQUALS(AnalysisDataService::Instance().doesExist(m_wsName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(m_wsName));
    TableWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<TableWorkspace>(output);
    TS_ASSERT_EQUALS(outputWS->columnCount(), 10);
    TS_ASSERT_EQUALS(outputWS->rowCount(), 10);

    // test the first three rows, equivalent to the first two rows of the file.
    TableRow row = outputWS->getRow(0);
    TS_ASSERT_EQUALS(row.cell<std::string>(1), "13460");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(2)), 0.7,
                    0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(3), "13463,13464");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)), 0.01,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)), 0.06,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(6)), 0.04,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(7)), 2,
                    0.01);
    TS_ASSERT_EQUALS(boost::lexical_cast<double>(row.cell<std::string>(0)), 1);

    row = outputWS->getRow(1);
    TS_ASSERT_EQUALS(row.cell<std::string>(1), "13469");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(2)), 0.7,
                    0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(3), "13463,13464");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)), 0.01,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)), 0.06,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(6)), 0.04,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(7)), 2,
                    0.01);
    TS_ASSERT_EQUALS(boost::lexical_cast<double>(row.cell<std::string>(0)), 2);

    row = outputWS->getRow(2);
    TS_ASSERT_EQUALS(row.cell<std::string>(1), "13470");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(2)), 2.3,
                    0.01);
    TS_ASSERT_EQUALS(row.cell<std::string>(3), "13463,13464");
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(4)),
                    0.035, 0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(5)), 0.3,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(6)), 0.04,
                    0.001);
    TS_ASSERT_DELTA(boost::lexical_cast<double>(row.cell<std::string>(7)), 2,
                    0.01);
    TS_ASSERT_EQUALS(boost::lexical_cast<double>(row.cell<std::string>(0)), 2);

    cleanupafterwards();
  }

  void testFewColumns() {
    // create a file with each line containing too few columns
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,\"13463,13464\",0.01,0.06,,,,,,,0.04,2\n";
    file << "13469,0.7,\"13463,13464\",0.01,0.06,13470,2.3,\"13463,13464\",0."
            "035,0.3,,0.04,2\n";
    file << "13460,0.7,\"13463,13464\",,\"13463,13464\",,13470,2.3,\"13463,"
            "13464\",0.035,0.04,2\n";
    file << "13460,0.7,\"13463,13464\",0.01,0.06,,13470,2.3,\"13463,13464\",0."
            "035,0.3,0.04,2\n";
    file << "13470,2.3,\"13463,13464\",0.035,0.3,,0.04,2\n";
    file << ",,,,13462,2.3,\"13463,13464\",0.035,0.3,0.04,2\n";
    file.close();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void testManyColumns() {
    // create a file with each line containing too many columns
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,13463,0.01,0.06,,,,,,,,,,,0.04,2,,,,0.04,2\n";
    file << "13469,0.7,13463,0.01,0.06,13470,2.3,13463,0.035,0.3,,,,,,0.04,2,,,"
            ",0.04,2\n";
    file << "13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,13470,2.3,"
            "13463,0.035,0.3,0.04,2,,,,0.04,2\n";
    file << "13460,0.7,13463,0.01,0.06,,,,,,13470,2.3,13463,0.035,0.3,0.04,2,,,"
            ",0.04,2\n";
    file << ",,,,,13470,2.3,13463,0.035,0.3,,,,,,0.04,2,,,,0.04,2\n";
    file << ",,,,,,,,,13462,2.3,13463,0.035,0.3,0.04,2,,,,0.04,2\n";
    file.close();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void testManyColumnsTwo() {
    // create a file with each line containing too many columns
    std::ofstream file(m_filename.c_str());
    file << "13460,0.7,\"13463,0.01\",0.06,,,,,,,,,,,0.04,2,,,,0.04,2\n";
    file << "13469,0.7,13463,\"0.01,0.06\",13470,2.3,13463,0.06,\"13470,0.06,"
            "13470\",2.3,13463,0.035,0.3,,,,,,,,,0.04,2,,,,0.04,2\n";
    file << "13460,0.7,13463,0.01,0.06,13462,2.3,13463,0.035,0.3,13470,2.3,"
            "13463,0.035,0.3,0.04,2,,,,0.04,2\n";
    file << "13460,0.7,\"13463,0.01\",0.06,,,,,,,,,,13470,2.3,\"13463,0.035\","
            "0.3,0.04,2,,,,0.04,2\n";
    file << ",,,,,13470,2.3,\"13463,0.035\",0.3,,,,,,,,,0.04,2,,,,0.04,2\n";
    file << ",,,,,,,,,,,,13462,2.3,\"13463,0.035\",0.3,0.04,2,,,,0.04,2\n";
    file.close();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());

    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void testTBLWithColumnHeadingsRowAndData() {
    // "New" TBL file with column headings

    std::ofstream file(m_filename.c_str());
    file << "Runs,Angle,QMin,QMax,Group,Options\n"
         << "14456,0.7,1.443,8.992,1,\n"
         << "18553,0.3,1.233,4.388,3,\n";
    file.close();
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().doesExist(m_wsName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(m_wsName));
    TableWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<TableWorkspace>(output);
    std::vector<std::string> cols{"Runs", "Angle", "QMin",
                                  "QMax", "Group", "Options"};
    TS_ASSERT_EQUALS(outputWS->getColumnNames(), cols);
    TableRow firstRow = outputWS->getRow(0);
    TS_ASSERT_EQUALS(firstRow.cell<std::string>(0), "14456");
    TS_ASSERT_EQUALS(firstRow.cell<std::string>(1), "0.7");
    TS_ASSERT_EQUALS(firstRow.cell<std::string>(3), "8.992");
    TS_ASSERT_EQUALS(firstRow.cell<std::string>(5), "");
    TableRow secondRow = outputWS->getRow(1);
    std::string entryOne = secondRow.cell<std::string>(0);
    std::string entryTwo = secondRow.cell<std::string>(1);
    std::string entryFour = secondRow.cell<std::string>(3);
    std::string entrySix = secondRow.cell<std::string>(5);
    TS_ASSERT_EQUALS(secondRow.cell<std::string>(0), "18553");
    TS_ASSERT_EQUALS(secondRow.cell<std::string>(1), "0.3");
    TS_ASSERT_EQUALS(secondRow.cell<std::string>(3), "4.388");

    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void testTBLWithColumnHeadingsRowOnly() {
    // "New" TBL file with column headings

    std::ofstream file(m_filename.c_str());
    file << "Runs,Angle,Transmission,Energy,Spin,Group,Options\n";
    file.close();
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT_EQUALS(AnalysisDataService::Instance().doesExist(m_wsName), true);
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(m_wsName));
    TableWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<TableWorkspace>(output);
    std::vector<std::string> cols{"Runs", "Angle", "Transmission", "Energy",
                                  "Spin", "Group", "Options"};
    TS_ASSERT_EQUALS(outputWS->getColumnNames(), cols);
    TableRow row = outputWS->getRow(0);
    // we added no rows so we should get runtime error when we try to acheive a
    // cell from a row that doesn't exist
    TS_ASSERT_THROWS_ANYTHING(row.cell<std::string>(0));
  }

  void testBlankFile() {
    std::ofstream file(m_filename.c_str());
    file.close();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);

    cleanupafterwards();
  }

  void testNoDataFile() {
    // create a file with content, and the right amount of delimiters, but no
    // valid data
    std::ofstream file(m_filename.c_str());
    file << ",,,,,,,,,,,,,,,,\n";
    file << ",,,,,,,,,,,,,,,,\n";
    file << ",,,,,,,,,,,,,,,,\n";
    file << ",,,,,,,,,,,,,,,,\n";
    file << ",,,,,,,,,,,,,,,,\n";
    file << ",,,,,,,,,,,,,,,,\n";
    file << ",,,,,,,,,,,,,,,,\n";
    file.close();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("LoadTBL");
    alg->setRethrows(true);
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", m_filename));
    m_abspath = alg->getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", m_wsName));
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    cleanupafterwards();
  }

private:
  const std::string m_filename;
  const std::string m_wsName;
  std::string m_abspath;
  void cleanupafterwards() {
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove(m_wsName));
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }
};

#endif // LOADTBLTEST_H_
