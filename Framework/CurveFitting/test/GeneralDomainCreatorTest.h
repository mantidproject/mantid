// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_GENERALDOMAINCREATORTEST_H_
#define MANTID_CURVEFITTING_GENERALDOMAINCREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/GeneralDomainCreator.h"

#include "MantidAPI/Column.h"
#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunctionGeneral.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PropertyManager.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::CurveFitting::GeneralDomainCreator;

namespace {

class TestFunction1 : public IFunctionGeneral, public ParamFunction {
public:
  TestFunction1() : IFunctionGeneral(), ParamFunction() {
    declareParameter("a", 1.0);
  }
  std::string name() const override { return "TestFunction1"; }
  size_t getNumberDomainColumns() const override { return 2; }
  size_t getNumberValuesPerArgument() const override { return 3; }
  void functionGeneral(const FunctionDomainGeneral &generalDomain,
                       FunctionValues &values) const override {
    double a = getParameter(0);
    auto x = generalDomain.getColumn(0);
    auto nam = generalDomain.getColumn(1);
    auto n = x->size();
    for (size_t i = 0; i < n; ++i) {
      double v = a * x->toDouble(i);
      if (nam->cell<std::string>(i) == "Beta") {
        v *= 2.0;
      }
      values.setCalculated(i, v);
      values.setCalculated(i + n, v / 10.);
      values.setCalculated(i + 2 * n, v / 100.);
    }
  }
};

class TestFunction2 : public IFunctionGeneral, public ParamFunction {
public:
  TestFunction2() : IFunctionGeneral(), ParamFunction() {
    declareParameter("a", 0.0);
  }
  std::string name() const override { return "TestFunction2"; }
  size_t getNumberDomainColumns() const override { return 0; }
  size_t getNumberValuesPerArgument() const override { return 2; }
  size_t getDefaultDomainSize() const override { return 5; }
  void functionGeneral(const FunctionDomainGeneral &,
                       FunctionValues &values) const override {
    double a = getParameter(0);
    auto n = getDefaultDomainSize();
    for (size_t i = 0; i < n; ++i) {
      values.setCalculated(i, a * double(i));
      values.setCalculated(i + n, a * (10.0 - double(i)));
    }
  }
};

} // anonymous namespace

class GeneralDomainCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GeneralDomainCreatorTest *createSuite() {
    return new GeneralDomainCreatorTest();
  }
  static void destroySuite(GeneralDomainCreatorTest *suite) { delete suite; }

  void test_declared_properties() {
    TestFunction1 fun;
    PropertyManager manager;
    GeneralDomainCreator creator(fun, manager, "InputWorkspace");
    creator.declareDatasetProperties();
    TS_ASSERT(manager.existsProperty("ArgumentColumn"));
    TS_ASSERT(manager.existsProperty("ArgumentColumn_1"));
    TS_ASSERT(!manager.existsProperty("ArgumentColumn_2"));
    TS_ASSERT(manager.existsProperty("DataColumn"));
    TS_ASSERT(manager.existsProperty("DataColumn_1"));
    TS_ASSERT(manager.existsProperty("DataColumn_2"));
    TS_ASSERT(!manager.existsProperty("DataColumn_3"));
    TS_ASSERT(manager.existsProperty("WeightsColumn"));
    TS_ASSERT(manager.existsProperty("WeightsColumn_1"));
    TS_ASSERT(manager.existsProperty("WeightsColumn_2"));
    TS_ASSERT(!manager.existsProperty("WeightsColumn_3"));
    TS_ASSERT_THROWS_NOTHING(creator.declareDatasetProperties("", false));
  }

  void test_domain_values() {
    TestFunction1 fun;
    PropertyManager manager;
    GeneralDomainCreator creator(fun, manager, "InputWorkspace");
    creator.declareDatasetProperties();

    TableWorkspace_sptr ws = makeData1();

    manager.declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                                "InputWorkspace", "", Direction::Input),
                            "Name of the input Workspace");
    manager.setProperty("InputWorkspace", ws);
    manager.setProperty("ArgumentColumn", "X");
    manager.setProperty("ArgumentColumn_1", "Name");
    manager.setProperty("DataColumn", "GoodData");
    manager.setProperty("DataColumn_1", "NotSoGoodData");
    manager.setProperty("DataColumn_2", "IgnoredData");
    manager.setProperty("WeightsColumn", "GoodDataW");
    manager.setProperty("WeightsColumn_1", "NotSoGoodDataW");
    manager.setProperty("WeightsColumn_2", "IgnoredDataW");

    TS_ASSERT_EQUALS(creator.getDomainSize(), 4);
    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    creator.createDomain(domain, values);
    TS_ASSERT_EQUALS(domain->size(), 4);
    TS_ASSERT_EQUALS(values->size(), 12);

    auto &generalDomain = static_cast<FunctionDomainGeneral &>(*domain);
    TS_ASSERT_EQUALS(generalDomain.size(), 4);
    TS_ASSERT_EQUALS(generalDomain.columnCount(), 2);
    auto column = generalDomain.getColumn(0);
    TS_ASSERT_EQUALS(column->size(), 4);
    TS_ASSERT_EQUALS(column->cell<double>(0), 1.0);
    TS_ASSERT_EQUALS(column->cell<double>(1), 1.0);
    TS_ASSERT_EQUALS(column->cell<double>(2), 3.0);
    TS_ASSERT_EQUALS(column->cell<double>(3), 3.0);

    column = generalDomain.getColumn(1);
    TS_ASSERT_EQUALS(column->size(), 4);
    TS_ASSERT_EQUALS(column->cell<std::string>(0), "Alpha");
    TS_ASSERT_EQUALS(column->cell<std::string>(1), "Beta");
    TS_ASSERT_EQUALS(column->cell<std::string>(2), "Alpha");
    TS_ASSERT_EQUALS(column->cell<std::string>(3), "Beta");

    TS_ASSERT_EQUALS(values->getFitData(0), 10.0);
    TS_ASSERT_EQUALS(values->getFitData(1), 20.0);
    TS_ASSERT_EQUALS(values->getFitData(2), 30.0);
    TS_ASSERT_EQUALS(values->getFitData(3), 60.0);
    TS_ASSERT_EQUALS(values->getFitData(4), 1.0);
    TS_ASSERT_EQUALS(values->getFitData(5), 2.0);
    TS_ASSERT_EQUALS(values->getFitData(6), 3.0);
    TS_ASSERT_EQUALS(values->getFitData(7), 6.0);
    TS_ASSERT_EQUALS(values->getFitData(8), 0.11);
    TS_ASSERT_EQUALS(values->getFitData(9), 0.22);
    TS_ASSERT_EQUALS(values->getFitData(10), 0.33);
    TS_ASSERT_EQUALS(values->getFitData(11), 0.66);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 0.1);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0.1);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 0.1);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 0.1);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 0.0);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 0.0);
    TS_ASSERT_EQUALS(values->getFitWeight(10), 0.0);
    TS_ASSERT_EQUALS(values->getFitWeight(11), 0.0);
  }

  void test_fit_ignore_3d_column() {
    IFunction_sptr fun(new TestFunction1);
    TableWorkspace_sptr ws = makeData1();

    Mantid::CurveFitting::Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("ArgumentColumn", "X");
    fit.setProperty("ArgumentColumn_1", "Name");
    fit.setProperty("DataColumn", "GoodData");
    fit.setProperty("DataColumn_1", "NotSoGoodData");
    fit.setProperty("DataColumn_2", "IgnoredData");
    fit.setProperty("WeightsColumn", "GoodDataW");
    fit.setProperty("WeightsColumn_1", "NotSoGoodDataW");
    fit.setProperty("WeightsColumn_2", "IgnoredDataW");

    TS_ASSERT_EQUALS(fun->getParameter(0), 1.0);
    fit.execute();
    TS_ASSERT_DELTA(fun->getParameter(0), 10.0, 1e-9);
  }

  void test_fit_include_3d_column() {
    IFunction_sptr fun(new TestFunction1);
    TableWorkspace_sptr ws = makeData1(10.0);

    Mantid::CurveFitting::Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("ArgumentColumn", "X");
    fit.setProperty("ArgumentColumn_1", "Name");
    fit.setProperty("DataColumn", "GoodData");
    fit.setProperty("DataColumn_1", "NotSoGoodData");
    fit.setProperty("DataColumn_2", "IgnoredData");
    fit.setProperty("WeightsColumn", "GoodDataW");
    fit.setProperty("WeightsColumn_1", "NotSoGoodDataW");
    fit.setProperty("WeightsColumn_2", "IgnoredDataW");

    TS_ASSERT_EQUALS(fun->getParameter(0), 1.0);
    fit.execute();
    TS_ASSERT_DELTA(fun->getParameter(0), 10.01, 2e-3);
  }

  void test_declared_properties_2() {
    TestFunction2 fun;
    PropertyManager manager;
    GeneralDomainCreator creator(fun, manager, "InputWorkspace");
    creator.declareDatasetProperties();
    TS_ASSERT(!manager.existsProperty("ArgumentColumn"));
    TS_ASSERT(!manager.existsProperty("ArgumentColumn_1"));

    TS_ASSERT(manager.existsProperty("DataColumn"));
    TS_ASSERT(manager.existsProperty("DataColumn_1"));
    TS_ASSERT(!manager.existsProperty("DataColumn_2"));
    TS_ASSERT(manager.existsProperty("WeightsColumn"));
    TS_ASSERT(manager.existsProperty("WeightsColumn_1"));
    TS_ASSERT(!manager.existsProperty("WeightsColumn_2"));
  }

  void test_domain_values_2() {
    TestFunction2 fun;
    PropertyManager manager;
    GeneralDomainCreator creator(fun, manager, "InputWorkspace");
    creator.declareDatasetProperties();

    TableWorkspace_sptr ws = makeData2();

    manager.declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                                "InputWorkspace", "", Direction::Input),
                            "Name of the input Workspace");
    manager.setProperty("InputWorkspace", ws);
    manager.setProperty("DataColumn", "Energies");
    manager.setProperty("DataColumn_1", "Intensities");
    manager.setProperty("WeightsColumn", "EnergiesW");
    manager.setProperty("WeightsColumn_1", "IntensitiesW");

    TS_ASSERT_EQUALS(creator.getDomainSize(), 5);
    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    creator.createDomain(domain, values);
    TS_ASSERT_EQUALS(domain->size(), 0);
    TS_ASSERT_EQUALS(values->size(), 10);

    TS_ASSERT_EQUALS(values->getFitData(0), 0.0);
    TS_ASSERT_EQUALS(values->getFitData(1), 1.0);
    TS_ASSERT_EQUALS(values->getFitData(2), 2.0);
    TS_ASSERT_EQUALS(values->getFitData(3), 3.0);
    TS_ASSERT_EQUALS(values->getFitData(4), 4.0);
    TS_ASSERT_EQUALS(values->getFitData(5), 10.0);
    TS_ASSERT_EQUALS(values->getFitData(6), 9.0);
    TS_ASSERT_EQUALS(values->getFitData(7), 8.0);
    TS_ASSERT_EQUALS(values->getFitData(8), 7.0);
    TS_ASSERT_EQUALS(values->getFitData(9), 6.0);

    TS_ASSERT_EQUALS(values->getFitWeight(0), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(1), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(2), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(3), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(4), 1.0);
    TS_ASSERT_EQUALS(values->getFitWeight(5), 0.5);
    TS_ASSERT_EQUALS(values->getFitWeight(6), 0.5);
    TS_ASSERT_EQUALS(values->getFitWeight(7), 0.5);
    TS_ASSERT_EQUALS(values->getFitWeight(8), 0.5);
    TS_ASSERT_EQUALS(values->getFitWeight(9), 0.5);
  }

  void test_fit_2() {
    IFunction_sptr fun(new TestFunction2);
    TableWorkspace_sptr ws = makeData2();

    Mantid::CurveFitting::Algorithms::Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("DataColumn", "Energies");
    fit.setProperty("DataColumn_1", "Intensities");
    fit.setProperty("WeightsColumn", "EnergiesW");
    fit.setProperty("WeightsColumn_1", "IntensitiesW");

    TS_ASSERT_EQUALS(fun->getParameter(0), 0.0);
    fit.execute();
    TS_ASSERT_DELTA(fun->getParameter(0), 1.0, 1e-9);
  }

  void test_create_output() {
    auto fun = boost::shared_ptr<TestFunction1>(new TestFunction1);
    PropertyManager manager;
    GeneralDomainCreator creator(*fun, manager, "InputWorkspace");
    creator.declareDatasetProperties();

    TableWorkspace_sptr ws = makeData1();

    manager.declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                                "InputWorkspace", "", Direction::Input),
                            "Name of the input Workspace");
    manager.setProperty("InputWorkspace", ws);
    manager.setProperty("ArgumentColumn", "X");
    manager.setProperty("ArgumentColumn_1", "Name");
    manager.setProperty("DataColumn", "GoodData");
    manager.setProperty("DataColumn_1", "NotSoGoodData");
    manager.setProperty("DataColumn_2", "IgnoredData");
    manager.setProperty("WeightsColumn", "GoodDataW");
    manager.setProperty("WeightsColumn_1", "NotSoGoodDataW");
    manager.setProperty("WeightsColumn_2", "IgnoredDataW");

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    creator.createDomain(domain, values);

    fun->setParameter(0, 5.0);
    fun->function(*domain, *values);

    auto result = boost::static_pointer_cast<ITableWorkspace>(
        creator.createOutputWorkspace("out", fun, domain, values));

    TS_ASSERT_EQUALS(result->columnCount(), 8);
    TS_ASSERT_EQUALS(result->rowCount(), 4);

    auto column = result->getColumn("X");
    TS_ASSERT_EQUALS(column->cell<double>(0), 1.0);
    TS_ASSERT_EQUALS(column->cell<double>(2), 3.0);

    column = result->getColumn("Name");
    TS_ASSERT_EQUALS(column->cell<std::string>(0), "Alpha");
    TS_ASSERT_EQUALS(column->cell<std::string>(2), "Alpha");

    column = result->getColumn("GoodData");
    TS_ASSERT_EQUALS(column->cell<double>(0), 10.0);
    TS_ASSERT_EQUALS(column->cell<double>(2), 30.0);

    column = result->getColumn("NotSoGoodData");
    TS_ASSERT_EQUALS(column->cell<double>(0), 1.0);
    TS_ASSERT_EQUALS(column->cell<double>(2), 3.0);

    column = result->getColumn("IgnoredData");
    TS_ASSERT_EQUALS(column->cell<double>(0), 0.11);
    TS_ASSERT_EQUALS(column->cell<double>(2), 0.33);

    column = result->getColumn("GoodData_calc");
    TS_ASSERT_EQUALS(column->cell<double>(0), 5.0);
    TS_ASSERT_EQUALS(column->cell<double>(2), 15.0);

    column = result->getColumn("NotSoGoodData_calc");
    TS_ASSERT_EQUALS(column->cell<double>(0), 0.5);
    TS_ASSERT_EQUALS(column->cell<double>(2), 1.5);

    column = result->getColumn("IgnoredData_calc");
    TS_ASSERT_EQUALS(column->cell<double>(0), 0.05);
    TS_ASSERT_EQUALS(column->cell<double>(2), 0.15);
  }

  void test_create_output_2() {
    auto fun = boost::shared_ptr<TestFunction2>(new TestFunction2);
    PropertyManager manager;
    GeneralDomainCreator creator(*fun, manager, "InputWorkspace");
    creator.declareDatasetProperties();

    TableWorkspace_sptr ws = makeData2();
    manager.declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                                "InputWorkspace", "", Direction::Input),
                            "Name of the input Workspace");
    manager.setProperty("InputWorkspace", ws);
    manager.setProperty("DataColumn", "Energies");
    manager.setProperty("DataColumn_1", "Intensities");
    manager.setProperty("WeightsColumn", "EnergiesW");
    manager.setProperty("WeightsColumn_1", "IntensitiesW");

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    creator.createDomain(domain, values);

    fun->setParameter(0, 2.0);
    fun->function(*domain, *values);

    auto result = boost::static_pointer_cast<ITableWorkspace>(
        creator.createOutputWorkspace("out", fun, domain, values));

    TS_ASSERT_EQUALS(result->columnCount(), 4);
    TS_ASSERT_EQUALS(result->rowCount(), 5);

    auto column = result->getColumn("Energies");
    TS_ASSERT_EQUALS(column->cell<double>(0), 0.0);
    TS_ASSERT_EQUALS(column->cell<double>(1), 1.0);
    TS_ASSERT_EQUALS(column->cell<double>(2), 2.0);

    column = result->getColumn("Intensities");
    TS_ASSERT_EQUALS(column->cell<double>(0), 10.0);
    TS_ASSERT_EQUALS(column->cell<double>(1), 9.0);
    TS_ASSERT_EQUALS(column->cell<double>(2), 8.0);

    column = result->getColumn("Energies_calc");
    TS_ASSERT_EQUALS(column->cell<double>(0), 0.0);
    TS_ASSERT_EQUALS(column->cell<double>(1), 2.0);
    TS_ASSERT_EQUALS(column->cell<double>(2), 4.0);

    column = result->getColumn("Intensities_calc");
    TS_ASSERT_EQUALS(column->cell<double>(0), 20.0);
    TS_ASSERT_EQUALS(column->cell<double>(1), 18.0);
    TS_ASSERT_EQUALS(column->cell<double>(2), 16.0);
  }

private:
  TableWorkspace_sptr makeData1(double wgt3col = 0.0) {
    TableWorkspace_sptr ws(new TableWorkspace);
    ws->addColumn("double", "X");
    ws->addColumn("str", "Name");

    ws->addColumn("double", "GoodData");
    ws->addColumn("double", "GoodDataW");
    ws->addColumn("double", "NotSoGoodData");
    ws->addColumn("double", "NotSoGoodDataW");
    ws->addColumn("double", "IgnoredData");
    ws->addColumn("double", "IgnoredDataW");

    TableRow row = ws->appendRow();
    row << 1.0 << "Alpha" << 10.0 << 1.0 << 1.0 << 0.1 << 0.11 << wgt3col;
    row = ws->appendRow();
    row << 1.0 << "Beta" << 20.0 << 1.0 << 2.0 << 0.1 << 0.22 << wgt3col;
    row = ws->appendRow();
    row << 3.0 << "Alpha" << 30.0 << 1.0 << 3.0 << 0.1 << 0.33 << wgt3col;
    row = ws->appendRow();
    row << 3.0 << "Beta" << 60.0 << 1.0 << 6.0 << 0.1 << 0.66 << wgt3col;

    return ws;
  }

  TableWorkspace_sptr makeData2() {
    TableWorkspace_sptr ws(new TableWorkspace);
    ws->addColumn("double", "Energies");
    ws->addColumn("double", "EnergiesW");
    ws->addColumn("double", "Intensities");
    ws->addColumn("double", "IntensitiesW");

    TableRow row = ws->appendRow();
    row << 0.0 << 1.0 << 10.0 << 0.5;
    row = ws->appendRow();
    row << 1.0 << 1.0 << 9.0 << 0.5;
    row = ws->appendRow();
    row << 2.0 << 1.0 << 8.0 << 0.5;
    row = ws->appendRow();
    row << 3.0 << 1.0 << 7.0 << 0.5;
    row = ws->appendRow();
    row << 4.0 << 1.0 << 6.0 << 0.5;

    return ws;
  }
};

#endif /* MANTID_CURVEFITTING_GENERALDOMAINCREATORTEST_H_ */
