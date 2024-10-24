// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <memory>
#include <vector>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/Property.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace std;

class TableWorkspaceAlgorithm : public Algorithm {
public:
  /// no arg constructor
  TableWorkspaceAlgorithm() : Algorithm() {}
  /// virtual destructor
  ~TableWorkspaceAlgorithm() override = default;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "TableWorkspaceAlgorithm"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Examples"; }
  const std::string summary() const override { return "Test summary"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Static reference to the logger class
  static Mantid::Kernel::Logger &g_log;
};

void TableWorkspaceAlgorithm::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Table", "", Direction::Input));
}

void TableWorkspaceAlgorithm::exec() {
  Workspace_sptr b = getProperty("Table");
  TableWorkspace_sptr t = std::dynamic_pointer_cast<TableWorkspace>(b);
  TableRow r = t->getFirstRow();
  r << "FIRST" << 11;
  r.next();
  r << "SECOND" << 22;
}

class TableWorkspacePropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TableWorkspacePropertyTest *createSuite() { return new TableWorkspacePropertyTest(); }
  static void destroySuite(TableWorkspacePropertyTest *suite) { delete suite; }

  TableWorkspacePropertyTest() {
    t.reset(new TableWorkspace(10));
    t->addColumn("str", "Name");
    t->addColumn("int", "Nunber");
    AnalysisDataService::Instance().add("tst", std::dynamic_pointer_cast<Workspace>(t));
  }

  void testProperty() {
    TableWorkspaceAlgorithm alg;
    alg.initialize();
    alg.setPropertyValue("Table", "tst");
    alg.execute();
    TableWorkspace_sptr table;
    TS_ASSERT_THROWS_NOTHING(table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>("tst"));
    TS_ASSERT(table);
    TS_ASSERT_EQUALS(table->rowCount(), 10);
    TableRow r = table->getFirstRow();
    std::string s;
    int n;
    r >> s >> n;
    TS_ASSERT_EQUALS(s, "FIRST");
    TS_ASSERT_EQUALS(n, 11);
    r.next();
    r >> s >> n;
    TS_ASSERT_EQUALS(s, "SECOND");
    TS_ASSERT_EQUALS(n, 22);
  }

private:
  std::shared_ptr<TableWorkspace> t;
};
