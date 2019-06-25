// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_MUONANALYSISRESULTTABLECREATORTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_MUONANALYSISRESULTTABLECREATORTEST_H_

#include "../Muon/MuonAnalysisResultTableCreator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QMap>
#include <QString>
#include <QVariant>
#include <cxxtest/TestSuite.h>

using Mantid::API::AlgorithmManager;
using Mantid::API::AnalysisDataService;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::TableRow;
using Mantid::API::WorkspaceFactory;
using Mantid::API::WorkspaceGroup;
using Mantid::API::Workspace_sptr;
using MantidQt::CustomInterfaces::LogValuesMap;
using MantidQt::CustomInterfaces::MuonAnalysisResultTableCreator;
using MantidQt::CustomInterfaces::WSParameterList;

/// This is a wrapper for the ADS that automatically clears itself on
/// destruction
class RAII_ADS {
public:
  void add(const std::string &name, const Workspace_sptr &ws) {
    AnalysisDataService::Instance().add(name, ws);
  }
  void addToGroup(const std::string &group, const std::string &name) {
    AnalysisDataService::Instance().addToGroup(group, name);
  }
  virtual ~RAII_ADS() { AnalysisDataService::Instance().clear(); }
};

/// Derived class to test protected methods
class TestCreator : public MuonAnalysisResultTableCreator {
public:
  TestCreator(const QStringList &items, const QStringList &logs,
              LogValuesMap *logValues)
      : MuonAnalysisResultTableCreator(items, logs, logValues, false) {}
  bool haveSameParameters(
      const std::vector<Mantid::API::ITableWorkspace_sptr> &tables) const {
    return MuonAnalysisResultTableCreator::haveSameParameters(tables);
  }
  void removeFixedParameterErrors(
      const Mantid::API::ITableWorkspace_sptr table) const {
    MuonAnalysisResultTableCreator::removeFixedParameterErrors(table);
  }
};

class MuonAnalysisResultTableCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisResultTableCreatorTest *createSuite() {
    return new MuonAnalysisResultTableCreatorTest();
  }
  static void destroySuite(MuonAnalysisResultTableCreatorTest *suite) {
    delete suite;
  }

  /// Constructor - set up log values
  MuonAnalysisResultTableCreatorTest() {
    Mantid::API::FrameworkManager::Instance();
    m_logValues = LogValuesMap();
    m_firstRun = 20918;
    m_firstStart_sec = 840710532;
    m_startDiff_sec = 1310;
  }

  void test_createTable_throws_noWorkspaces() {
    const QStringList workspaces;
    this->setUpLogs(QStringList{"EMU00020918; Pair; long; Asym; #1",
                                "EMU00020919; Pair; long; Asym; #1"});
    MuonAnalysisResultTableCreator creator(workspaces, m_logs, &m_logValues,
                                           false);
    TS_ASSERT_THROWS(creator.createTable(), const std::runtime_error &);
  }

  void test_createTable_throws_noLogs() {
    const QStringList workspaces{"EMU00020918; Pair; long; Asym; #1",
                                 "EMU00020919; Pair; long; Asym; #1"};

    this->setUpLogs(workspaces);
    MuonAnalysisResultTableCreator creator(workspaces, QStringList(),
                                           &m_logValues, false);
    TS_ASSERT_THROWS(creator.createTable(), const std::runtime_error &);
  }

  /// Two separate fits of one run each
  void test_createTable_singleFits() {
    // Set up
    const QStringList workspaces{"EMU00020918; Pair; long; Asym; #1",
                                 "EMU00020919; Pair; long; Asym; #1"};
    this->setUpLogs(workspaces);
    RAII_ADS ads;
    ads.add("EMU00020918; Pair; long; Asym; #1_Workspace",
            getWorkspace(m_firstRun));
    ads.add("EMU00020919; Pair; long; Asym; #1_Workspace",
            getWorkspace(m_firstRun + 1));
    ads.add("EMU00020918; Pair; long; Asym; #1_Parameters",
            getParamTable(m_firstRun));
    ads.add("EMU00020919; Pair; long; Asym; #1_Parameters",
            getParamTable(m_firstRun + 1));
    ads.add("EMU00020918", boost::make_shared<WorkspaceGroup>());
    ads.add("EMU00020919", boost::make_shared<WorkspaceGroup>());
    ads.addToGroup("EMU00020918",
                   "EMU00020918; Pair; long; Asym; #1_Workspace");
    ads.addToGroup("EMU00020918",
                   "EMU00020918; Pair; long; Asym; #1_Parameters");
    ads.addToGroup("EMU00020919",
                   "EMU00020919; Pair; long; Asym; #1_Workspace");
    ads.addToGroup("EMU00020919",
                   "EMU00020919; Pair; long; Asym; #1_Parameters");

    // Test
    MuonAnalysisResultTableCreator creator(workspaces, m_logs, &m_logValues,
                                           false);
    ITableWorkspace_sptr resultTable;
    TS_ASSERT_THROWS_NOTHING(resultTable = creator.createTable());
    TS_ASSERT(resultTable);
    const auto &expected = getExpectedOutputSingle(workspaces);
    // compare workspaces
    TS_ASSERT(compareTables(resultTable, expected));
  }

  void test_createTable_singleFits_differentModels_throws() {
    // Set up
    const QStringList workspaces{"EMU00020918; Pair; long; Asym; #1",
                                 "EMU00020919; Pair; long; Asym; #1"};
    this->setUpLogs(workspaces);
    RAII_ADS ads;
    ads.add("EMU00020918; Pair; long; Asym; #1_Workspace",
            getWorkspace(m_firstRun));
    ads.add("EMU00020919; Pair; long; Asym; #1_Workspace",
            getWorkspace(m_firstRun + 1));
    ads.add("EMU00020918; Pair; long; Asym; #1_Parameters",
            getParamTable(m_firstRun));
    ads.add("EMU00020919; Pair; long; Asym; #1_Parameters",
            getAlternateParamTable());
    ads.add("EMU00020918", boost::make_shared<WorkspaceGroup>());
    ads.add("EMU00020919", boost::make_shared<WorkspaceGroup>());
    ads.addToGroup("EMU00020918",
                   "EMU00020918; Pair; long; Asym; #1_Workspace");
    ads.addToGroup("EMU00020918",
                   "EMU00020918; Pair; long; Asym; #1_Parameters");
    ads.addToGroup("EMU00020919",
                   "EMU00020919; Pair; long; Asym; #1_Workspace");
    ads.addToGroup("EMU00020919",
                   "EMU00020919; Pair; long; Asym; #1_Parameters");

    // Test
    MuonAnalysisResultTableCreator creator(workspaces, m_logs, &m_logValues,
                                           false);
    TS_ASSERT_THROWS(creator.createTable(), const std::runtime_error &);
  }

  /// Sequential fit of two runs
  void test_createTable_sequentialFit() {
    // Set up
    const QStringList workspaces{"MuonSeqFit_Label_EMU20918",
                                 "MuonSeqFit_Label_EMU20919"};
    this->setUpLogs(workspaces);
    RAII_ADS ads;
    ads.add("MuonSeqFit_Label_EMU20918_Parameters", getParamTable(m_firstRun));
    ads.add("MuonSeqFit_Label_EMU20918_Workspace", getWorkspace(m_firstRun));
    ads.add("MuonSeqFit_Label_EMU20919_Parameters",
            getParamTable(m_firstRun + 1));
    ads.add("MuonSeqFit_Label_EMU20919_Workspace",
            getWorkspace(m_firstRun + 1));
    ads.add("MuonSeqFit_Label", boost::make_shared<WorkspaceGroup>());
    ads.addToGroup("MuonSeqFit_Label", "MuonSeqFit_Label_EMU20918_Parameters");
    ads.addToGroup("MuonSeqFit_Label", "MuonSeqFit_Label_EMU20918_Workspace");
    ads.addToGroup("MuonSeqFit_Label", "MuonSeqFit_Label_EMU20919_Parameters");
    ads.addToGroup("MuonSeqFit_Label", "MuonSeqFit_Label_EMU20919_Workspace");

    // Test
    MuonAnalysisResultTableCreator creator(workspaces, m_logs, &m_logValues,
                                           false);
    ITableWorkspace_sptr resultTable;
    TS_ASSERT_THROWS_NOTHING(resultTable = creator.createTable());
    TS_ASSERT(resultTable);
    const auto &expected = getExpectedOutputSingle(workspaces);
    // compare workspaces
    TS_ASSERT(compareTables(resultTable, expected));
  }

  /// Simultaneous fit of two runs
  void test_createTable_simultaneousFit() {
    // Set up
    const QStringList workspaces{"MuonSimulFit_Label_EMU20918_long",
                                 "MuonSimulFit_Label_EMU20919_long"};
    this->setUpLogs(workspaces);
    RAII_ADS ads;
    ads.add("MuonSimulFit_Label_EMU20918_long_Parameters",
            getParamTable(m_firstRun));
    ads.add("MuonSimulFit_Label_EMU20918_long_Workspace",
            getWorkspace(m_firstRun));
    ads.add("MuonSimulFit_Label_EMU20919_long_Parameters",
            getParamTable(m_firstRun + 1));
    ads.add("MuonSimulFit_Label_EMU20919_long_Workspace",
            getWorkspace(m_firstRun + 1));
    ads.add("MuonSimulFit_Label", boost::make_shared<WorkspaceGroup>());
    ads.addToGroup("MuonSimulFit_Label",
                   "MuonSimulFit_Label_EMU20918_long_Parameters");
    ads.addToGroup("MuonSimulFit_Label",
                   "MuonSimulFit_Label_EMU20918_long_Workspace");
    ads.addToGroup("MuonSimulFit_Label",
                   "MuonSimulFit_Label_EMU20919_long_Parameters");
    ads.addToGroup("MuonSimulFit_Label",
                   "MuonSimulFit_Label_EMU20919_long_Workspace");

    // Test
    MuonAnalysisResultTableCreator creator(workspaces, m_logs, &m_logValues,
                                           false);
    ITableWorkspace_sptr resultTable;
    TS_ASSERT_THROWS_NOTHING(resultTable = creator.createTable());
    TS_ASSERT(resultTable);
    const auto &expected = getExpectedOutputSingle(workspaces);
    // compare workspaces
    TS_ASSERT(compareTables(resultTable, expected));
  }

  /// Table of results from multiple simultaneous fits
  void test_createTable_multiple() {
    // Set up
    const QStringList labels{"Label", "Label#2"};
    const std::vector<std::vector<std::string>> runs = {
        {"_EMU20918", "_EMU20919"}, {"_EMU20920", "_EMU20921"}};
    const QStringList workspaces{"MuonSimulFit_Label_EMU20918_long",
                                 "MuonSimulFit_Label_EMU20919_long",
                                 "MuonSimulFit_Label#2_EMU20920_long",
                                 "MuonSimulFit_Label#2_EMU20921_long"};
    this->setUpLogs(workspaces);
    RAII_ADS ads;
    for (int i = 0; i < labels.size(); ++i) {
      const std::string &prefix = "MuonSimulFit_" + labels[i].toStdString();
      ads.add(prefix + runs[i][0] + "_long_Parameters",
              getParamTable(m_firstRun));
      ads.add(prefix + runs[i][0] + "_long_Workspace",
              getWorkspace(m_firstRun));
      ads.add(prefix + runs[i][1] + "_long_Parameters",
              getParamTable(m_firstRun + 1, true));
      ads.add(prefix + runs[i][1] + "_long_Workspace",
              getWorkspace(m_firstRun + 1));
      ads.add(prefix, boost::make_shared<WorkspaceGroup>());
      ads.addToGroup(prefix, prefix + runs[i][0] + "_long_Parameters");
      ads.addToGroup(prefix, prefix + runs[i][0] + "_long_Workspace");
      ads.addToGroup(prefix, prefix + runs[i][1] + "_long_Parameters");
      ads.addToGroup(prefix, prefix + runs[i][1] + "_long_Workspace");
    }

    // Test
    MuonAnalysisResultTableCreator creator(labels, m_logs, &m_logValues, true);
    ITableWorkspace_sptr resultTable;
    TS_ASSERT_THROWS_NOTHING(resultTable = creator.createTable());
    TS_ASSERT(resultTable);
    const auto &expected = getExpectedOutputMultiple();
    // compare workspaces
    TS_ASSERT(compareTables(resultTable, expected));
  }

  void test_createTable_multiple_throws_differentNumberDatasets() {
    // Set up
    const QStringList labels{"Label", "Label#2"};
    const std::vector<std::vector<std::string>> runs = {
        {"_EMU20918", "_EMU20919"}, {"_EMU20920", "_EMU20921", "_EMU20923"}};
    const QStringList workspaces{"MuonSimulFit_Label_EMU20918_long",
                                 "MuonSimulFit_Label_EMU20919_long",
                                 "MuonSimulFit_Label#2_EMU20920_long",
                                 "MuonSimulFit_Label#2_EMU20921_long",
                                 "MuonSimulFit_Label#2_EMU20923_long"};
    this->setUpLogs(workspaces);
    RAII_ADS ads;
    for (int i = 0; i < labels.size(); ++i) {
      const std::string &prefix = "MuonSimulFit_" + labels[i].toStdString();
      ads.add(prefix, boost::make_shared<WorkspaceGroup>());
      for (size_t j = 0; j < runs[i].size(); ++j) {
        const int run = m_firstRun + static_cast<int>(j);
        ads.add(prefix + runs[i][j] + "_long_Parameters",
                getParamTable(run, j > 0));
        ads.add(prefix + runs[i][j] + "_long_Workspace", getWorkspace(run));
        ads.addToGroup(prefix, prefix + runs[i][j] + "_long_Parameters");
        ads.addToGroup(prefix, prefix + runs[i][j] + "_long_Workspace");
      }
    }

    // Test
    MuonAnalysisResultTableCreator creator(labels, m_logs, &m_logValues, true);
    TS_ASSERT_THROWS(creator.createTable(), const std::runtime_error &);
  }

  void test_haveSameParameters_Yes() {
    const QStringList workspaces{"EMU00020918; Pair; long; Asym; #1",
                                 "EMU00020919; Pair; long; Asym; #1"};
    this->setUpLogs(workspaces);
    TestCreator creator(workspaces, m_logs, &m_logValues);

    const auto &tableOne = getParamTable(m_firstRun);
    const auto &tableTwo = getParamTable(m_firstRun + 1);
    const auto &tableThree = getParamTable(m_firstRun + 2);
    bool sameParams;
    TS_ASSERT_THROWS_NOTHING(sameParams = creator.haveSameParameters(
                                 {tableOne, tableTwo, tableThree}));
    TS_ASSERT_EQUALS(true, sameParams);
  }

  void test_haveSameParameters_No() {
    const QStringList workspaces{"EMU00020918; Pair; long; Asym; #1",
                                 "EMU00020919; Pair; long; Asym; #1"};
    this->setUpLogs(workspaces);
    TestCreator creator(workspaces, m_logs, &m_logValues);

    const auto &tableOne = getParamTable(m_firstRun);
    const auto &tableTwo = getParamTable(m_firstRun + 1);
    const auto &tableThree = getAlternateParamTable();
    bool sameParams;
    TS_ASSERT_THROWS_NOTHING(sameParams = creator.haveSameParameters(
                                 {tableOne, tableTwo, tableThree}));
    TS_ASSERT_EQUALS(false, sameParams);
  }

  void test_removeFixedParameterErrors() {
    const QStringList workspaces{"EMU00020918; Pair; long; Asym; #1",
                                 "EMU00020919; Pair; long; Asym; #1"};
    this->setUpLogs(workspaces);
    TestCreator creator(workspaces, m_logs, &m_logValues);

    // Create table in function so that we will no longer have shared ownership
    // of its columns when they are deleted
    const auto table = []() {
      auto tab = WorkspaceFactory::Instance().createTable();
      tab->addColumn("str", "Run");
      tab->addColumn("double", "A0");
      tab->addColumn("double", "A0Error");
      tab->addColumn("double", "A1");
      tab->addColumn("double", "A1Error");
      tab->addColumn("double", "Cost function");
      tab->addColumn("double", "Cost function Error");

      TableRow row1 = tab->appendRow();
      TableRow row2 = tab->appendRow();
      TableRow row3 = tab->appendRow();

      row1 << "15189" << 2.5 << 0.0 << 3.0 << 0.0 << 0.5 << 0.0;
      row2 << "15190" << 2.2 << 0.3 << 3.2 << 0.0 << 0.3 << 0.0;
      row3 << "15191" << 2.3 << 0.2 << 3.1 << 0.0 << 0.4 << 0.0;
      return tab;
    }();

    TS_ASSERT_THROWS_NOTHING(creator.removeFixedParameterErrors(table));

    TS_ASSERT_EQUALS(5, table->columnCount());
    const auto names = table->getColumnNames();
    const std::vector<std::string> expectedNames{"Run", "A0", "A0Error", "A1",
                                                 "Cost function"};
    TS_ASSERT_EQUALS(names, expectedNames);
  }

private:
  /// Set up log values for test
  void setUpLogs(const QStringList &names) {
    m_logValues.clear();
    int run = m_firstRun;
    int start = m_firstStart_sec;
    int temp = 200;
    int mag = 100;
    for (int i = 0; i < names.size(); ++i) {
      QMap<QString, QVariant> values;
      // strings
      values["run_number"] = QVariant(QString::number(run));
      // double - range
      values["sample_temp"] = QVariant(temp);
      // double - same
      values["sample_magn_field"] = QVariant(mag);
      // time in seconds since 1990
      values["run_start (s)"] = QVariant(start);

      m_logValues[names[i]] = values;

      run++;
      temp -= 10;
      start += m_startDiff_sec;
    }

    m_logs = m_logValues.begin()->keys();
  }

  /// Create parameter table output from a fit
  ITableWorkspace_sptr getParamTable(int runNumber, bool makeGlobals = false) {
    const double base = static_cast<double>(runNumber - m_firstRun);
    auto table = WorkspaceFactory::Instance().createTable();
    // Create columns
    table->addColumn("str", "Name");
    table->addColumn("double", "Y");
    table->addColumn("double", "Error");

    // Add a row for each parameter
    TableRow a0row = table->appendRow();
    TableRow arow = table->appendRow();
    TableRow omegarow = table->appendRow();
    TableRow phirow = table->appendRow();
    TableRow sigrow = table->appendRow();
    TableRow taurow = table->appendRow();
    TableRow cfrow = table->appendRow();

    // Fill table with data
    constexpr double zero(0.0), error(0.1), a0(0.1), a(0.2), omega(0.3),
        phi(0.4), sigma(0.5), tau(0.6);
    a0row << "f0.A0" << base + a0 << error;
    taurow << "f1.Tau" << base + tau << error;
    cfrow << "Cost function value" << 0.03 << 0.0;

    if (makeGlobals) {
      arow << "f1.A" << a << zero;
      omegarow << "f1.Omega" << omega << zero;
      phirow << "f1.Phi" << phi << zero;
      sigrow << "f1.Sigma" << sigma << zero;

    } else {
      arow << "f1.A" << base + a << error;
      omegarow << "f1.Omega" << base + omega << zero; // param fixed: zero error
      phirow << "f1.Phi" << base + phi << error;
      sigrow << "f1.Sigma" << base + sigma << error;
    }

    return table;
  }

  /// Create parameter table output from a fit
  ITableWorkspace_sptr getAlternateParamTable() {
    auto table = WorkspaceFactory::Instance().createTable();
    // Create columns
    table->addColumn("str", "Name");
    table->addColumn("double", "Y");
    table->addColumn("double", "Error");

    // Add a row for each parameter
    TableRow a0row = table->appendRow();
    TableRow arow = table->appendRow();
    TableRow lambdarow = table->appendRow();
    TableRow cfrow = table->appendRow();

    // Fill table with data
    constexpr double zero(0.0), error(0.1), a0(0.1), a(0.2), lambda(0.3);
    a0row << "f0.A0" << a0 << error;
    arow << "f1.A" << a << error;
    lambdarow << "f1.Tau" << lambda << error;
    cfrow << "Cost function value" << 0.03 << zero;

    return table;
  }

  /// Expected output table
  ITableWorkspace_sptr getExpectedOutputSingle(const QStringList workspaces) {
    auto table = WorkspaceFactory::Instance().createTable();
    table->addColumn("str", "workspace_Name");
    const std::vector<std::string> titles = {
        "f0.A0",
        "f0.A0Error",
        "f1.A",
        "f1.AError",
        "f1.Omega", // no omega error as param is fixed
        "f1.Phi",
        "f1.PhiError",
        "f1.Sigma",
        "f1.SigmaError",
        "f1.Tau",
        "f1.TauError",
        "Cost function value"};
    for (const auto &log : m_logs) {
      table->addColumn("double", log.toStdString());
    }
    for (const auto &title : titles) {
      table->addColumn("double", title);
    }

    constexpr double err(0.1);
    TableRow firstRow = table->appendRow();
    TableRow secondRow = table->appendRow();

    firstRow << workspaces[0].toStdString() << std::stod("20918")
             << std::stod("0") << std::stod("100") << std::stod("200") << 0.1
             << err << 0.2 << err << 0.3 << 0.4 << err << 0.5 << err << 0.6
             << err << 0.03;
    secondRow << workspaces[1].toStdString() << std::stod("20919")
              << std::stod(std::to_string(m_startDiff_sec)) << std::stod("100")
              << std::stod("190") << 1.1 << err << 1.2 << err << 1.3 << 1.4
              << err << 1.5 << err << 1.6 << err << 0.03;
    return table;
  }

  /// Expected output table from multiple simultaneous fits
  ITableWorkspace_sptr getExpectedOutputMultiple() {
    auto table = WorkspaceFactory::Instance().createTable();

    const std::vector<std::string> titles = {
        "f0.f0.A0",      "f0.f0.A0Error",  "f1.f0.A0",
        "f1.f0.A0Error", "f1.A",           "f1.AError",
        "f1.Omega", // no omega error as param was fixed
        "f1.Phi",        "f1.PhiError",    "f1.Sigma",
        "f1.SigmaError", "f0.f1.Tau",      "f0.f1.TauError",
        "f1.f1.Tau",     "f1.f1.TauError", "Cost function value"};
    table->addColumn("str", "Label");
    int k = 0;
    for (const auto &log : m_logs) {
      if (k == 2) {

        table->addColumn("double", log.toStdString());
      } else {
        table->addColumn("str", log.toStdString());
      }
      k++;
    }
    for (const auto &title : titles) {
      table->addColumn("double", title);
    }
    constexpr double err(0.1);
    TableRow firstRow = table->appendRow();
    TableRow secondRow = table->appendRow();

    firstRow << "Label"
             << "20918-20919"
             << "0-1310" << 100.0 << "190-200" << 0.1 << err << 1.1 << err
             << 0.2 << err << 0.3 << 0.4 << err << 0.5 << err << 0.6 << err
             << 1.6 << err << 0.03;
    secondRow << "Label#2"
              << "20920-20921"
              << "2620-3930" << 100.0 << "170-180" << 0.1 << err << 1.1 << err
              << 0.2 << err << 0.3 << 0.4 << err << 0.5 << err << 0.6 << err
              << 1.6 << err << 0.03;

    return table;
  }

  bool compareTables(const ITableWorkspace_sptr lhs,
                     const ITableWorkspace_sptr rhs) {
    auto alg = AlgorithmManager::Instance().create("CompareWorkspaces");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("Workspace1", lhs);
    alg->setProperty("Workspace2", rhs);
    alg->execute();
    bool result = alg->getProperty("Result");
    return result;
  }

  MatrixWorkspace_sptr getWorkspace(int runNumber) {
    const int base = runNumber - m_firstRun;
    const double start =
        static_cast<double>(m_firstStart_sec + base * m_startDiff_sec) * 1.e9;
    const double end = start + 1.e10;
    const auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    ws->mutableRun().setStartAndEndTime(static_cast<int64_t>(start),
                                        static_cast<int64_t>(end));
    return ws;
  }

  QStringList m_logs;
  LogValuesMap m_logValues;
  int m_firstStart_sec, m_startDiff_sec, m_firstRun;
};

#endif /* MANTIDQT_CUSTOMINTERFACES_MUONANALYSISRESULTTABLECREATORTEST_H_ */
