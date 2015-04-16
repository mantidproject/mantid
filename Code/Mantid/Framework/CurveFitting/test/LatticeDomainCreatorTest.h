#ifndef MANTID_CURVEFITTING_LATTICEDOMAINCREATORTEST_H_
#define MANTID_CURVEFITTING_LATTICEDOMAINCREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/LatticeDomain.h"
#include "MantidCurveFitting/LatticeDomainCreator.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::CurveFitting::LatticeDomainCreator;
using Mantid::Kernel::V3D;

using namespace Mantid::API;
using namespace Mantid::DataObjects;

class LatticeDomainCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LatticeDomainCreatorTest *createSuite() {
    return new LatticeDomainCreatorTest();
  }
  static void destroySuite(LatticeDomainCreatorTest *suite) { delete suite; }

  void testGetDomainSizeIPeaksWorkspace() {
    Workspace_sptr peaksWs = boost::dynamic_pointer_cast<Workspace>(
        WorkspaceCreationHelper::createPeaksWorkspace(5));

    TestableLatticeDomainCreator dc;
    TS_ASSERT_THROWS_NOTHING(dc.setWorkspace(peaksWs));

    TS_ASSERT_EQUALS(dc.getDomainSize(), 5);
  }

  void testGetDomainSizeTableWorkspace() {
    ITableWorkspace_sptr table = getValidTableWs();

    TestableLatticeDomainCreator dc;
    TS_ASSERT_THROWS_NOTHING(dc.setWorkspace(table));

    TS_ASSERT_EQUALS(dc.getDomainSize(), 3);
  }

  void testCreateDomainTableWs() {
    ITableWorkspace_sptr table = getValidTableWs();

    TestableLatticeDomainCreator dc;

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    TS_ASSERT_THROWS_NOTHING(
        dc.createDomainFromPeakTable(table, domain, values, 0));

    TS_ASSERT_EQUALS(domain->size(), 3);
    TS_ASSERT_EQUALS(values->size(), 3);

    boost::shared_ptr<LatticeDomain> latticeDomain =
        boost::dynamic_pointer_cast<LatticeDomain>(domain);

    TS_ASSERT(latticeDomain);

    TS_ASSERT_EQUALS((*latticeDomain)[0], V3D(1, 1, 1));
    TS_ASSERT_EQUALS((*latticeDomain)[1], V3D(2, 2, 0));
    TS_ASSERT_EQUALS((*latticeDomain)[2], V3D(3, 1, 1));

    TS_ASSERT_EQUALS(values->getFitData(0), 3.135702);
    TS_ASSERT_EQUALS(values->getFitData(1), 1.920217);
    TS_ASSERT_EQUALS(values->getFitData(2), 1.637567);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1.0);
  }

  void testCreateDomainTableWsInvalid() {
    ITableWorkspace_sptr invalid = getInvalidTableWs();
    ITableWorkspace_sptr empty = getEmptyTableWs();

    TestableLatticeDomainCreator dc;

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    TS_ASSERT_THROWS(dc.createDomainFromPeakTable(invalid, domain, values, 0),
                     std::runtime_error);

    TS_ASSERT_THROWS(dc.createDomainFromPeakTable(empty, domain, values, 0),
                     std::range_error);
  }

  void testCreateDomainPeaksWorkspace() {
    IPeaksWorkspace_sptr peaksWs = boost::dynamic_pointer_cast<IPeaksWorkspace>(
        WorkspaceCreationHelper::createPeaksWorkspace(2));
    peaksWs->getPeak(0).setHKL(V3D(1, 1, 1));
    peaksWs->getPeak(1).setHKL(V3D(2, 2, 0));

    TestableLatticeDomainCreator dc;

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    TS_ASSERT_THROWS_NOTHING(
        dc.createDomainFromPeaksWorkspace(peaksWs, domain, values, 0));

    TS_ASSERT_EQUALS(domain->size(), 2);
    TS_ASSERT_EQUALS(values->size(), 2);

    boost::shared_ptr<LatticeDomain> latticeDomain =
        boost::dynamic_pointer_cast<LatticeDomain>(domain);

    TS_ASSERT(latticeDomain);

    TS_ASSERT_EQUALS((*latticeDomain)[0], V3D(1, 1, 1));
    TS_ASSERT_EQUALS((*latticeDomain)[1], V3D(2, 2, 0));

    TS_ASSERT_EQUALS(values->getFitData(0), peaksWs->getPeak(0).getDSpacing());
    TS_ASSERT_EQUALS(values->getFitData(1), peaksWs->getPeak(1).getDSpacing());

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1.0);
  }

  void testCreateOutputWorkspace() {
    ITableWorkspace_sptr table = getValidTableWs();

    IFunction_sptr fn;
    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    // Make domain and values
    TestableLatticeDomainCreator dc;
    dc.createDomainFromPeakTable(table, domain, values, 0);

    values->setCalculated(0, 3.125702);
    values->setCalculated(1, 1.930217);
    values->setCalculated(2, 1.627567);

    Workspace_sptr outputWs;
    TS_ASSERT_THROWS_NOTHING(
        outputWs = dc.createOutputWorkspace("base", fn, domain, values, "pp"));

    ITableWorkspace_sptr tableWs =
        boost::dynamic_pointer_cast<ITableWorkspace>(outputWs);
    TS_ASSERT(tableWs);

    TS_ASSERT_EQUALS(tableWs->rowCount(), 3);
    TS_ASSERT_DELTA(tableWs->cell<double>(0, 3), 0.01, 1e-6);
    TS_ASSERT_DELTA(tableWs->cell<double>(1, 3), -0.01, 1e-6);
    TS_ASSERT_DELTA(tableWs->cell<double>(2, 3), 0.01, 1e-6);
  }

private:
  ITableWorkspace_sptr getValidTableWs() {
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
    table->addColumn("V3D", "HKL");
    table->addColumn("double", "d");

    TableRow newRow = table->appendRow();
    newRow << V3D(1, 1, 1) << 3.135702;
    newRow = table->appendRow();
    newRow << V3D(2, 2, 0) << 1.920217;
    newRow = table->appendRow();
    newRow << V3D(3, 1, 1) << 1.637567;

    return table;
  }

  ITableWorkspace_sptr getEmptyTableWs() {
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
    table->addColumn("V3D", "HKL");
    table->addColumn("double", "d");

    return table;
  }

  ITableWorkspace_sptr getInvalidTableWs() {
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
    table->addColumn("V3D", "HKL");

    TableRow newRow = table->appendRow();
    newRow << V3D(1, 1, 1);
    newRow = table->appendRow();
    newRow << V3D(2, 2, 0);
    newRow = table->appendRow();
    newRow << V3D(3, 1, 1);

    return table;
  }

  class TestableLatticeDomainCreator : public LatticeDomainCreator {
    friend class LatticeDomainCreatorTest;

  public:
    TestableLatticeDomainCreator() : LatticeDomainCreator(NULL, "") {}

    void setWorkspace(const Workspace_sptr &ws) { m_workspace = ws; }
  };
};

#endif /* MANTID_CURVEFITTING_LATTICEDOMAINCREATORTEST_H_ */
