#ifndef MANTID_CURVEFITTING_CRYSTALFIELDPEAKSTEST_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/TableRow.h"
#include "MantidCurveFitting/Algorithms/EvaluateFunction.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"
#include "MantidDataObjects/TableWorkspace.h"

using Mantid::CurveFitting::Functions::CrystalFieldPeaks;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CrystalFieldPeaksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CrystalFieldPeaksTest *createSuite() {
    return new CrystalFieldPeaksTest();
  }
  static void destroySuite(CrystalFieldPeaksTest *suite) { delete suite; }

  void test_calculation() {
    CrystalFieldPeaks fun;
    FunctionDomainGeneral domain;
    FunctionValues values;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);
    fun.setAttributeValue("ToleranceIntensity", 0.001);
    fun.function(domain, values);

    TS_ASSERT_EQUALS(values.size(), 6);
    TS_ASSERT_DELTA(values[0], 0.0, 0.01);
    TS_ASSERT_DELTA(values[1], 29.33, 0.01);
    TS_ASSERT_DELTA(values[2], 44.34, 0.01);
    TS_ASSERT_DELTA(values[3], 2.75, 0.01);
    TS_ASSERT_DELTA(values[4], 0.72, 0.01);
    TS_ASSERT_DELTA(values[5], 0.43, 0.01);
  }

  void test_fit() {
    IFunction_sptr fun(new CrystalFieldPeaks);
    FunctionDomainGeneral domain;
    FunctionValues values;
    fun->fixAll();
    fun->setParameter("B20", 0.37);
    fun->setParameter("B22", 3.9);
    fun->setParameter("B40", -0.03);
    fun->setParameter("B42", -0.11);
    fun->setParameter("B44", -0.12);
    fun->unfixParameter("B20");
    fun->unfixParameter("B22");
    fun->unfixParameter("B40");
    fun->unfixParameter("B42");
    fun->unfixParameter("B44");
    fun->setAttributeValue("Ion", "Ce");
    fun->setAttributeValue("Temperature", 44.0);
    fun->setAttributeValue("ToleranceIntensity", 0.001);

    auto data = TableWorkspace_sptr(new TableWorkspace);
    data->addColumn("double", "Energy");
    data->addColumn("double", "Intensity");

    TableRow row = data->appendRow();
    row << 0.0 << 2.74937;
    row = data->appendRow();
    row << 29.3261 << 0.7204;
    row = data->appendRow();
    row << 44.3412 << 0.429809;

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", data);
    fit.setProperty("DataColumn", "Energy");
    fit.setProperty("DataColumn_1", "Intensity");
    fit.setProperty("Output", "out");
    fit.execute();

    fun->function(domain, values);

    TS_ASSERT_DELTA(values[0], 0.0, 0.0001);
    TS_ASSERT_DELTA(values[1], 29.3261, 0.00005);
    TS_ASSERT_DELTA(values[2], 44.3412, 0.00005);
    TS_ASSERT_DELTA(values[3], 2.74937, 0.000005);
    TS_ASSERT_DELTA(values[4], 0.7204, 0.00005);
    TS_ASSERT_DELTA(values[5], 0.429809, 0.0000005);

    TS_ASSERT_DELTA(fun->getParameter("B20"), 0.366336, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("B22"), 3.98132, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("B40"), -0.0304001, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("B42"), -0.119605, 0.0001);
    TS_ASSERT_DELTA(fun->getParameter("B44"), -0.130124, 0.0001);

    ITableWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            "out_Workspace");
    TS_ASSERT(output);
    if (output) {
      TS_ASSERT_EQUALS(output->rowCount(), 3);
      TS_ASSERT_EQUALS(output->columnCount(), 4);
      auto column = output->getColumn("Energy");
      TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(1), 29.3261, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(2), 44.3412, 0.0001);
      column = output->getColumn("Intensity");
      TS_ASSERT_DELTA(column->toDouble(0), 2.74937, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(1), 0.7204, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(2), 0.429809, 0.0001);
      column = output->getColumn("Energy_calc");
      TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(1), 29.3261, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(2), 44.3412, 0.0001);
      column = output->getColumn("Intensity_calc");
      TS_ASSERT_DELTA(column->toDouble(0), 2.74937, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(1), 0.7204, 0.0001);
      TS_ASSERT_DELTA(column->toDouble(2), 0.429809, 0.0001);
    }

    AnalysisDataService::Instance().clear();
  }

  void test_factory() {
    std::string ini =
        "name=CrystalFieldPeaks,Ion=Ce,Temperature=25.0,B20=1,B22="
        "2,B40=3,B44=4,ties=(B42=B44/2)";
    auto fun = FunctionFactory::Instance().createInitialized(ini);
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->nParams(), 34);
    TS_ASSERT_EQUALS(fun->nAttributes(), 5);
    fun->applyTies();
    TS_ASSERT_DELTA(fun->getParameter("B20"), 1.0, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter("B22"), 2.0, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter("B40"), 3.0, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter("B42"), 2.0, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter("B44"), 4.0, 1e-10);
    TS_ASSERT_EQUALS(fun->getAttribute("Ion").asString(), "Ce");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperature").asDouble(), 25.0);
    TS_ASSERT_EQUALS(fun->getAttribute("ToleranceEnergy").asDouble(), 1e-10);
    TS_ASSERT_EQUALS(fun->getAttribute("ToleranceIntensity").asDouble(), 1e-3);
  }

  void test_evaluate_alg_no_input_workspace() {
    IFunction_sptr fun(new CrystalFieldPeaks);
    FunctionDomainGeneral domain;
    FunctionValues values;

    fun->setParameter("B20", 0.37737);
    fun->setParameter("B22", 3.9770);
    fun->setParameter("B40", -0.031787);
    fun->setParameter("B42", -0.11611);
    fun->setParameter("B44", -0.12544);
    fun->setAttributeValue("Ion", "Ce");
    fun->setAttributeValue("Temperature", 44.0);
    fun->setAttributeValue("ToleranceIntensity", 0.001);

    EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun);
    eval.removeProperty("InputWorkspace");
    eval.setProperty("OutputWorkspace", "out");
    eval.execute();

    ITableWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("out");
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->rowCount(), 3);
    TS_ASSERT_EQUALS(output->columnCount(), 2);
    auto column = output->getColumn("DataColumn");
    TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(1), 29.3261, 0.00005);
    TS_ASSERT_DELTA(column->toDouble(2), 44.3412, 0.00005);
    column = output->getColumn("DataColumn_1");
    TS_ASSERT_DELTA(column->toDouble(0), 2.74937, 0.000005);
    TS_ASSERT_DELTA(column->toDouble(1), 0.7204, 0.00005);
    TS_ASSERT_DELTA(column->toDouble(2), 0.429809, 0.0000005);

    AnalysisDataService::Instance().clear();
  }

  void test_evaluate_alg_set_input_workspace() {
    IFunction_sptr fun(new CrystalFieldPeaks);
    FunctionDomainGeneral domain;
    FunctionValues values;

    fun->setParameter("B20", 0.37);
    fun->setParameter("B22", 3.9);
    fun->setParameter("B40", -0.03);
    fun->setParameter("B42", -0.11);
    fun->setParameter("B44", -0.12);
    fun->setAttributeValue("Ion", "Ce");
    fun->setAttributeValue("Temperature", 44.0);
    fun->setAttributeValue("ToleranceIntensity", 0.001);

    auto data = TableWorkspace_sptr(new TableWorkspace);
    data->addColumn("double", "Energy");
    data->addColumn("double", "Intensity");

    TableRow row = data->appendRow();
    row << 0.0 << 2.74937;
    row = data->appendRow();
    row << 29.3261 << 0.7204;
    row = data->appendRow();
    row << 44.3412 << 0.429809;

    EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun);
    eval.setProperty("InputWorkspace", data);
    eval.setProperty("DataColumn", "Energy");
    eval.setProperty("DataColumn_1", "Intensity");
    eval.setProperty("OutputWorkspace", "out");
    eval.execute();

    ITableWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("out");
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->rowCount(), 3);
    TS_ASSERT_EQUALS(output->columnCount(), 4);
    auto column = output->getColumn("Energy");
    TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(1), 29.3261, 0.00005);
    TS_ASSERT_DELTA(column->toDouble(2), 44.3412, 0.00005);
    column = output->getColumn("Intensity");
    TS_ASSERT_DELTA(column->toDouble(0), 2.74937, 0.000005);
    TS_ASSERT_DELTA(column->toDouble(1), 0.7204, 0.00005);
    TS_ASSERT_DELTA(column->toDouble(2), 0.429809, 0.0000005);
    column = output->getColumn("Energy_calc");
    TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(1), 28.7149, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(2), 43.3162, 0.0001);
    column = output->getColumn("Intensity_calc");
    TS_ASSERT_DELTA(column->toDouble(0), 2.7483, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(1), 0.7394, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(2), 0.4116, 0.0001);

    AnalysisDataService::Instance().clear();
  }

  void test_symmetry() {
    CrystalFieldPeaks fun;
    fun.setAttributeValue("Ion", "Ce");
    TS_ASSERT(!isFixed(fun, "B20"));
    TS_ASSERT(!isFixed(fun, "B21"));
    TS_ASSERT(isFixed(fun, "IB21"));
    TS_ASSERT(!isFixed(fun, "B22"));
    TS_ASSERT(!isFixed(fun, "IB22"));

    TS_ASSERT(!isFixed(fun, "B40"));
    TS_ASSERT(!isFixed(fun, "B41"));
    TS_ASSERT(!isFixed(fun, "IB41"));
    TS_ASSERT(!isFixed(fun, "B42"));
    TS_ASSERT(!isFixed(fun, "IB42"));
    TS_ASSERT(!isFixed(fun, "B43"));
    TS_ASSERT(!isFixed(fun, "IB43"));
    TS_ASSERT(!isFixed(fun, "B44"));
    TS_ASSERT(!isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(!isFixed(fun, "B61"));
    TS_ASSERT(!isFixed(fun, "IB61"));
    TS_ASSERT(!isFixed(fun, "B62"));
    TS_ASSERT(!isFixed(fun, "IB62"));
    TS_ASSERT(!isFixed(fun, "B63"));
    TS_ASSERT(!isFixed(fun, "IB63"));
    TS_ASSERT(!isFixed(fun, "B64"));
    TS_ASSERT(!isFixed(fun, "IB64"));
    TS_ASSERT(!isFixed(fun, "B65"));
    TS_ASSERT(!isFixed(fun, "IB65"));
    TS_ASSERT(!isFixed(fun, "B66"));
    TS_ASSERT(!isFixed(fun, "IB66"));

    setSymmetry(fun, "C2");
    TS_ASSERT(!isFixed(fun, "B20"));
    TS_ASSERT(isFixed(fun, "B21"));
    TS_ASSERT(isFixed(fun, "IB21"));
    TS_ASSERT(!isFixed(fun, "B22"));
    TS_ASSERT(isFixed(fun, "IB22"));

    TS_ASSERT(!isFixed(fun, "B40"));
    TS_ASSERT(isFixed(fun, "B41"));
    TS_ASSERT(isFixed(fun, "IB41"));
    TS_ASSERT(!isFixed(fun, "B42"));
    TS_ASSERT(!isFixed(fun, "IB42"));
    TS_ASSERT(isFixed(fun, "B43"));
    TS_ASSERT(isFixed(fun, "IB43"));
    TS_ASSERT(!isFixed(fun, "B44"));
    TS_ASSERT(!isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(isFixed(fun, "B61"));
    TS_ASSERT(isFixed(fun, "IB61"));
    TS_ASSERT(!isFixed(fun, "B62"));
    TS_ASSERT(!isFixed(fun, "IB62"));
    TS_ASSERT(isFixed(fun, "B63"));
    TS_ASSERT(isFixed(fun, "IB63"));
    TS_ASSERT(!isFixed(fun, "B64"));
    TS_ASSERT(!isFixed(fun, "IB64"));
    TS_ASSERT(isFixed(fun, "B65"));
    TS_ASSERT(isFixed(fun, "IB65"));
    TS_ASSERT(!isFixed(fun, "B66"));
    TS_ASSERT(!isFixed(fun, "IB66"));

    setSymmetry(fun, "C2v");
    TS_ASSERT(!isFixed(fun, "B20"));
    TS_ASSERT(isFixed(fun, "B21"));
    TS_ASSERT(isFixed(fun, "IB21"));
    TS_ASSERT(!isFixed(fun, "B22"));
    TS_ASSERT(isFixed(fun, "IB22"));

    TS_ASSERT(!isFixed(fun, "B40"));
    TS_ASSERT(isFixed(fun, "B41"));
    TS_ASSERT(isFixed(fun, "IB41"));
    TS_ASSERT(!isFixed(fun, "B42"));
    TS_ASSERT(isFixed(fun, "IB42"));
    TS_ASSERT(isFixed(fun, "B43"));
    TS_ASSERT(isFixed(fun, "IB43"));
    TS_ASSERT(!isFixed(fun, "B44"));
    TS_ASSERT(isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(isFixed(fun, "B61"));
    TS_ASSERT(isFixed(fun, "IB61"));
    TS_ASSERT(!isFixed(fun, "B62"));
    TS_ASSERT(isFixed(fun, "IB62"));
    TS_ASSERT(isFixed(fun, "B63"));
    TS_ASSERT(isFixed(fun, "IB63"));
    TS_ASSERT(!isFixed(fun, "B64"));
    TS_ASSERT(isFixed(fun, "IB64"));
    TS_ASSERT(isFixed(fun, "B65"));
    TS_ASSERT(isFixed(fun, "IB65"));
    TS_ASSERT(!isFixed(fun, "B66"));
    TS_ASSERT(isFixed(fun, "IB66"));

    setSymmetry(fun, "C4");
    TS_ASSERT(!isFixed(fun, "B20"));
    TS_ASSERT(isFixed(fun, "B21"));
    TS_ASSERT(isFixed(fun, "IB21"));
    TS_ASSERT(isFixed(fun, "B22"));
    TS_ASSERT(isFixed(fun, "IB22"));

    TS_ASSERT(!isFixed(fun, "B40"));
    TS_ASSERT(isFixed(fun, "B41"));
    TS_ASSERT(isFixed(fun, "IB41"));
    TS_ASSERT(isFixed(fun, "B42"));
    TS_ASSERT(isFixed(fun, "IB42"));
    TS_ASSERT(isFixed(fun, "B43"));
    TS_ASSERT(isFixed(fun, "IB43"));
    TS_ASSERT(!isFixed(fun, "B44"));
    TS_ASSERT(isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(isFixed(fun, "B61"));
    TS_ASSERT(isFixed(fun, "IB61"));
    TS_ASSERT(isFixed(fun, "B62"));
    TS_ASSERT(isFixed(fun, "IB62"));
    TS_ASSERT(isFixed(fun, "B63"));
    TS_ASSERT(isFixed(fun, "IB63"));
    TS_ASSERT(!isFixed(fun, "B64"));
    TS_ASSERT(!isFixed(fun, "IB64"));
    TS_ASSERT(isFixed(fun, "B65"));
    TS_ASSERT(isFixed(fun, "IB65"));
    TS_ASSERT(isFixed(fun, "B66"));
    TS_ASSERT(isFixed(fun, "IB66"));

    setSymmetry(fun, "D4");
    TS_ASSERT(!isFixed(fun, "B20"));
    TS_ASSERT(isFixed(fun, "B21"));
    TS_ASSERT(isFixed(fun, "IB21"));
    TS_ASSERT(isFixed(fun, "B22"));
    TS_ASSERT(isFixed(fun, "IB22"));

    TS_ASSERT(!isFixed(fun, "B40"));
    TS_ASSERT(isFixed(fun, "B41"));
    TS_ASSERT(isFixed(fun, "IB41"));
    TS_ASSERT(isFixed(fun, "B42"));
    TS_ASSERT(isFixed(fun, "IB42"));
    TS_ASSERT(isFixed(fun, "B43"));
    TS_ASSERT(isFixed(fun, "IB43"));
    TS_ASSERT(!isFixed(fun, "B44"));
    TS_ASSERT(isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(isFixed(fun, "B61"));
    TS_ASSERT(isFixed(fun, "IB61"));
    TS_ASSERT(isFixed(fun, "B62"));
    TS_ASSERT(isFixed(fun, "IB62"));
    TS_ASSERT(isFixed(fun, "B63"));
    TS_ASSERT(isFixed(fun, "IB63"));
    TS_ASSERT(!isFixed(fun, "B64"));
    TS_ASSERT(isFixed(fun, "IB64"));
    TS_ASSERT(isFixed(fun, "B65"));
    TS_ASSERT(isFixed(fun, "IB65"));
    TS_ASSERT(isFixed(fun, "B66"));
    TS_ASSERT(isFixed(fun, "IB66"));

    setSymmetry(fun, "C3");
    TS_ASSERT(!isFixed(fun, "B20"));
    TS_ASSERT(isFixed(fun, "B21"));
    TS_ASSERT(isFixed(fun, "IB21"));
    TS_ASSERT(isFixed(fun, "B22"));
    TS_ASSERT(isFixed(fun, "IB22"));

    TS_ASSERT(!isFixed(fun, "B40"));
    TS_ASSERT(isFixed(fun, "B41"));
    TS_ASSERT(isFixed(fun, "IB41"));
    TS_ASSERT(isFixed(fun, "B42"));
    TS_ASSERT(isFixed(fun, "IB42"));
    TS_ASSERT(!isFixed(fun, "B43"));
    TS_ASSERT(isFixed(fun, "IB43"));
    TS_ASSERT(isFixed(fun, "B44"));
    TS_ASSERT(isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(isFixed(fun, "B61"));
    TS_ASSERT(isFixed(fun, "IB61"));
    TS_ASSERT(isFixed(fun, "B62"));
    TS_ASSERT(isFixed(fun, "IB62"));
    TS_ASSERT(!isFixed(fun, "B63"));
    TS_ASSERT(!isFixed(fun, "IB63"));
    TS_ASSERT(isFixed(fun, "B64"));
    TS_ASSERT(isFixed(fun, "IB64"));
    TS_ASSERT(isFixed(fun, "B65"));
    TS_ASSERT(isFixed(fun, "IB65"));
    TS_ASSERT(!isFixed(fun, "B66"));
    TS_ASSERT(!isFixed(fun, "IB66"));

    setSymmetry(fun, "D3");
    TS_ASSERT(!isFixed(fun, "B20"));
    TS_ASSERT(isFixed(fun, "B21"));
    TS_ASSERT(isFixed(fun, "IB21"));
    TS_ASSERT(isFixed(fun, "B22"));
    TS_ASSERT(isFixed(fun, "IB22"));

    TS_ASSERT(!isFixed(fun, "B40"));
    TS_ASSERT(isFixed(fun, "B41"));
    TS_ASSERT(isFixed(fun, "IB41"));
    TS_ASSERT(isFixed(fun, "B42"));
    TS_ASSERT(isFixed(fun, "IB42"));
    TS_ASSERT(!isFixed(fun, "B43"));
    TS_ASSERT(isFixed(fun, "IB43"));
    TS_ASSERT(isFixed(fun, "B44"));
    TS_ASSERT(isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(isFixed(fun, "B61"));
    TS_ASSERT(isFixed(fun, "IB61"));
    TS_ASSERT(isFixed(fun, "B62"));
    TS_ASSERT(isFixed(fun, "IB62"));
    TS_ASSERT(!isFixed(fun, "B63"));
    TS_ASSERT(isFixed(fun, "IB63"));
    TS_ASSERT(isFixed(fun, "B64"));
    TS_ASSERT(isFixed(fun, "IB64"));
    TS_ASSERT(isFixed(fun, "B65"));
    TS_ASSERT(isFixed(fun, "IB65"));
    TS_ASSERT(!isFixed(fun, "B66"));
    TS_ASSERT(isFixed(fun, "IB66"));

    setSymmetry(fun, "C6");
    TS_ASSERT(!isFixed(fun, "B20"));
    TS_ASSERT(isFixed(fun, "B21"));
    TS_ASSERT(isFixed(fun, "IB21"));
    TS_ASSERT(isFixed(fun, "B22"));
    TS_ASSERT(isFixed(fun, "IB22"));

    TS_ASSERT(!isFixed(fun, "B40"));
    TS_ASSERT(isFixed(fun, "B41"));
    TS_ASSERT(isFixed(fun, "IB41"));
    TS_ASSERT(isFixed(fun, "B42"));
    TS_ASSERT(isFixed(fun, "IB42"));
    TS_ASSERT(isFixed(fun, "B43"));
    TS_ASSERT(isFixed(fun, "IB43"));
    TS_ASSERT(isFixed(fun, "B44"));
    TS_ASSERT(isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(isFixed(fun, "B61"));
    TS_ASSERT(isFixed(fun, "IB61"));
    TS_ASSERT(isFixed(fun, "B62"));
    TS_ASSERT(isFixed(fun, "IB62"));
    TS_ASSERT(isFixed(fun, "B63"));
    TS_ASSERT(isFixed(fun, "IB63"));
    TS_ASSERT(isFixed(fun, "B64"));
    TS_ASSERT(isFixed(fun, "IB64"));
    TS_ASSERT(isFixed(fun, "B65"));
    TS_ASSERT(isFixed(fun, "IB65"));
    TS_ASSERT(!isFixed(fun, "B66"));
    TS_ASSERT(isFixed(fun, "IB66"));

    fun.setAttributeValue("Symmetry", "T");
    TS_ASSERT(isFixed(fun, "B20"));
    TS_ASSERT(isFixed(fun, "B21"));
    TS_ASSERT(isFixed(fun, "IB21"));
    TS_ASSERT(isFixed(fun, "B22"));
    TS_ASSERT(isFixed(fun, "IB22"));

    TS_ASSERT(!isFixed(fun, "B40"));
    TS_ASSERT(isFixed(fun, "B41"));
    TS_ASSERT(isFixed(fun, "IB41"));
    TS_ASSERT(isFixed(fun, "B42"));
    TS_ASSERT(isFixed(fun, "IB42"));
    TS_ASSERT(isFixed(fun, "B43"));
    TS_ASSERT(isFixed(fun, "IB43"));
    auto i = fun.parameterIndex("B44");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(isFixed(fun, "B61"));
    TS_ASSERT(isFixed(fun, "IB61"));
    TS_ASSERT(isFixed(fun, "B62"));
    TS_ASSERT(isFixed(fun, "IB62"));
    TS_ASSERT(isFixed(fun, "B63"));
    TS_ASSERT(isFixed(fun, "IB63"));
    i = fun.parameterIndex("B64");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(isFixed(fun, "IB64"));
    TS_ASSERT(isFixed(fun, "B65"));
    TS_ASSERT(isFixed(fun, "IB65"));
    TS_ASSERT(isFixed(fun, "B66"));
    TS_ASSERT(isFixed(fun, "IB66"));

    i = fun.parameterIndex("B44");
    auto tie = fun.getTie(i);
    TS_ASSERT_EQUALS(tie->asString(), "B44=5*B40");

    i = fun.parameterIndex("B64");
    tie = fun.getTie(i);
    TS_ASSERT_EQUALS(tie->asString(), "B64=-21*B60");
  }

private:
  bool isFixed(const IFunction &fun, const std::string &par) {
    auto i = fun.parameterIndex(par);
    return fun.isFixed(i) && fun.getParameter(i) == 0.0;
  }

  void setSymmetry(IFunction &fun, const std::string &symm) {
    for (size_t i = 0; i < fun.nParams(); ++i) {
      fun.setParameter(i, 1.0);
    }
    fun.setAttributeValue("Symmetry", symm);
  }
};

#endif /* MANTID_CURVEFITTING_CRYSTALFIELDPEAKSTEST_H_ */
