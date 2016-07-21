#ifndef MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_
#define MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/HistogramDomainCreator.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidKernel/PropertyManager.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::CurveFitting::HistogramDomainCreator;

class HistogramDomainCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramDomainCreatorTest *createSuite() {
    return new HistogramDomainCreatorTest();
  }
  static void destroySuite(HistogramDomainCreatorTest *suite) { delete suite; }

  void test_declared_properties() {
    //TestFunction1 fun;
    //PropertyManager manager;
    //GeneralDomainCreator creator(fun, manager, "InputWorkspace");
    //creator.declareDatasetProperties();
    //TS_ASSERT(manager.existsProperty("ArgumentColumn"));
    //TS_ASSERT(manager.existsProperty("ArgumentColumn_1"));
    //TS_ASSERT(!manager.existsProperty("ArgumentColumn_2"));
    //TS_ASSERT(manager.existsProperty("DataColumn"));
    //TS_ASSERT(manager.existsProperty("DataColumn_1"));
    //TS_ASSERT(manager.existsProperty("DataColumn_2"));
    //TS_ASSERT(!manager.existsProperty("DataColumn_3"));
    //TS_ASSERT(manager.existsProperty("WeightsColumn"));
    //TS_ASSERT(manager.existsProperty("WeightsColumn_1"));
    //TS_ASSERT(manager.existsProperty("WeightsColumn_2"));
    //TS_ASSERT(!manager.existsProperty("WeightsColumn_3"));
    //TS_ASSERT_THROWS_NOTHING(creator.declareDatasetProperties("", false));
  }

  void test_domain_values() {
    //TestFunction1 fun;
    //PropertyManager manager;
    //GeneralDomainCreator creator(fun, manager, "InputWorkspace");
    //creator.declareDatasetProperties();

    //TableWorkspace_sptr ws = makeData1();

    //manager.declareProperty(make_unique<WorkspaceProperty<Workspace>>(
    //                            "InputWorkspace", "", Direction::Input),
    //                        "Name of the input Workspace");
    //manager.setProperty("InputWorkspace", ws);
    //manager.setProperty("ArgumentColumn", "X");
    //manager.setProperty("ArgumentColumn_1", "Name");
    //manager.setProperty("DataColumn", "GoodData");
    //manager.setProperty("DataColumn_1", "NotSoGoodData");
    //manager.setProperty("DataColumn_2", "IgnoredData");
    //manager.setProperty("WeightsColumn", "GoodDataW");
    //manager.setProperty("WeightsColumn_1", "NotSoGoodDataW");
    //manager.setProperty("WeightsColumn_2", "IgnoredDataW");

    //TS_ASSERT_EQUALS(creator.getDomainSize(), 4);
    //FunctionDomain_sptr domain;
    //FunctionValues_sptr values;
    //creator.createDomain(domain, values);
    //TS_ASSERT_EQUALS(domain->size(), 4);
    //TS_ASSERT_EQUALS(values->size(), 12);

    //auto &generalDomain = static_cast<FunctionDomainGeneral &>(*domain);
    //TS_ASSERT_EQUALS(generalDomain.size(), 4);
    //TS_ASSERT_EQUALS(generalDomain.columnCount(), 2);
    //auto column = generalDomain.getColumn(0);
    //TS_ASSERT_EQUALS(column->size(), 4);
    //TS_ASSERT_EQUALS(column->cell<double>(0), 1.0);
    //TS_ASSERT_EQUALS(column->cell<double>(1), 1.0);
    //TS_ASSERT_EQUALS(column->cell<double>(2), 3.0);
    //TS_ASSERT_EQUALS(column->cell<double>(3), 3.0);

    //column = generalDomain.getColumn(1);
    //TS_ASSERT_EQUALS(column->size(), 4);
    //TS_ASSERT_EQUALS(column->cell<std::string>(0), "Alpha");
    //TS_ASSERT_EQUALS(column->cell<std::string>(1), "Beta");
    //TS_ASSERT_EQUALS(column->cell<std::string>(2), "Alpha");
    //TS_ASSERT_EQUALS(column->cell<std::string>(3), "Beta");

    //TS_ASSERT_EQUALS(values->getFitData(0), 10.0);
    //TS_ASSERT_EQUALS(values->getFitData(1), 20.0);
    //TS_ASSERT_EQUALS(values->getFitData(2), 30.0);
    //TS_ASSERT_EQUALS(values->getFitData(3), 60.0);
    //TS_ASSERT_EQUALS(values->getFitData(4), 1.0);
    //TS_ASSERT_EQUALS(values->getFitData(5), 2.0);
    //TS_ASSERT_EQUALS(values->getFitData(6), 3.0);
    //TS_ASSERT_EQUALS(values->getFitData(7), 6.0);
    //TS_ASSERT_EQUALS(values->getFitData(8), 0.11);
    //TS_ASSERT_EQUALS(values->getFitData(9), 0.22);
    //TS_ASSERT_EQUALS(values->getFitData(10), 0.33);
    //TS_ASSERT_EQUALS(values->getFitData(11), 0.66);

    //TS_ASSERT_EQUALS(values->getFitWeight(0), 1.0);
    //TS_ASSERT_EQUALS(values->getFitWeight(1), 1.0);
    //TS_ASSERT_EQUALS(values->getFitWeight(2), 1.0);
    //TS_ASSERT_EQUALS(values->getFitWeight(3), 1.0);
    //TS_ASSERT_EQUALS(values->getFitWeight(4), 0.1);
    //TS_ASSERT_EQUALS(values->getFitWeight(5), 0.1);
    //TS_ASSERT_EQUALS(values->getFitWeight(6), 0.1);
    //TS_ASSERT_EQUALS(values->getFitWeight(7), 0.1);
    //TS_ASSERT_EQUALS(values->getFitWeight(8), 0.0);
    //TS_ASSERT_EQUALS(values->getFitWeight(9), 0.0);
    //TS_ASSERT_EQUALS(values->getFitWeight(10), 0.0);
    //TS_ASSERT_EQUALS(values->getFitWeight(11), 0.0);
  }

private:
};

#endif /* MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_ */
