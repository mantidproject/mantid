// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/TableRow.h"
#include "MantidCurveFitting/Algorithms/EvaluateFunction.h"
#include "MantidCurveFitting/EigenFortranDefs.h"
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
  static CrystalFieldPeaksTest *createSuite() { return new CrystalFieldPeaksTest(); }
  static void destroySuite(CrystalFieldPeaksTest *suite) { delete suite; }

  // Conversion factor from barn to milibarn/steradian
  const double c_mbsr = 79.5774715459;

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
    fun.setAttributeValue("ToleranceIntensity", 0.001 * c_mbsr);
    fun.function(domain, values);

    TS_ASSERT_EQUALS(values.size(), 6);
    TS_ASSERT_DELTA(values[0], 0.0, 0.01);
    TS_ASSERT_DELTA(values[1], 29.33, 0.01);
    TS_ASSERT_DELTA(values[2], 44.34, 0.01);
    TS_ASSERT_DELTA(values[3], 2.75 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(values[4], 0.72 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(values[5], 0.43 * c_mbsr, 0.001 * c_mbsr);
  }

  void test_further_calculation() {

    CrystalFieldPeaks fun;
    FunctionDomainGeneral domain;
    FunctionValues values;
    fun.setParameter("B20", 0.366336);
    fun.setParameter("B22", 3.98132);
    fun.setParameter("B40", -0.0304001);
    fun.setParameter("B42", -0.119605);
    fun.setParameter("B44", -0.130124);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);
    fun.setAttributeValue("ToleranceIntensity", 0.001 * c_mbsr);
    fun.function(domain, values);

    TS_ASSERT_DELTA(values[0], 0.0, 0.0001);
    TS_ASSERT_DELTA(values[1], 29.3261, 0.00005);
    TS_ASSERT_DELTA(values[2], 44.3412, 0.00005);
    TS_ASSERT_DELTA(values[3], 2.74937 * c_mbsr, 0.000005 * c_mbsr);
    TS_ASSERT_DELTA(values[4], 0.7204 * c_mbsr, 0.00005 * c_mbsr);
    TS_ASSERT_DELTA(values[5], 0.429809 * c_mbsr, 0.000005 * c_mbsr);
  }

  void test_factory() {
    std::string ini = "name=CrystalFieldPeaks,Ion=Ce,Temperature=25.0,B20=1,B22="
                      "2,B40=3,B44=4,ties=(B42=B44/2)";
    auto fun = FunctionFactory::Instance().createInitialized(ini);
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->nParams(), 34);
    TS_ASSERT_EQUALS(fun->nAttributes(), 6);
    fun->applyTies();
    TS_ASSERT_DELTA(fun->getParameter("B20"), 1.0, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter("B22"), 2.0, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter("B40"), 3.0, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter("B42"), 2.0, 1e-10);
    TS_ASSERT_DELTA(fun->getParameter("B44"), 4.0, 1e-10);
    TS_ASSERT_EQUALS(fun->getAttribute("Ion").asString(), "Ce");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperature").asDouble(), 25.0);
    TS_ASSERT_EQUALS(fun->getAttribute("ToleranceEnergy").asDouble(), 1e-10);
    TS_ASSERT_EQUALS(fun->getAttribute("ToleranceIntensity").asDouble(), 1e-1);
  }

  void test_arbitrary_J() {
    IFunction_sptr fun(new CrystalFieldPeaks);
    Mantid::CurveFitting::DoubleFortranVector en;
    Mantid::CurveFitting::ComplexFortranMatrix wf;
    int nre = 1;
    auto &peaks = dynamic_cast<CrystalFieldPeaks &>(*fun);
    peaks.setParameter("B20", 0.37737);
    peaks.setAttributeValue("Temperature", 44.0);
    peaks.setAttributeValue("ToleranceIntensity", 0.001 * c_mbsr);
    peaks.setAttributeValue("Ion", "something");
    TS_ASSERT_THROWS(peaks.calculateEigenSystem(en, wf, nre), const std::runtime_error &);
    peaks.setAttributeValue("Ion", "S2.4");
    TS_ASSERT_THROWS(peaks.calculateEigenSystem(en, wf, nre), const std::runtime_error &);
    peaks.setAttributeValue("Ion", "S2.5");
    TS_ASSERT_THROWS_NOTHING(peaks.calculateEigenSystem(en, wf, nre));
    TS_ASSERT_EQUALS(nre, -5);
    peaks.setAttributeValue("Ion", "s1");
    TS_ASSERT_THROWS_NOTHING(peaks.calculateEigenSystem(en, wf, nre));
    TS_ASSERT_EQUALS(nre, -2);
    peaks.setAttributeValue("Ion", "j1.5");
    TS_ASSERT_THROWS_NOTHING(peaks.calculateEigenSystem(en, wf, nre));
    TS_ASSERT_EQUALS(nre, -3);
    peaks.setAttributeValue("Ion", "J2");
    TS_ASSERT_THROWS_NOTHING(peaks.calculateEigenSystem(en, wf, nre));
    TS_ASSERT_EQUALS(nre, -4);
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
    fun->setAttributeValue("ToleranceIntensity", 0.001 * c_mbsr);

    EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun);
    eval.removeProperty("InputWorkspace");
    eval.setProperty("OutputWorkspace", "out");
    eval.execute();
    TS_ASSERT(eval.isExecuted());
    if (!eval.isExecuted()) {
      return;
    }

    ITableWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("out");
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->rowCount(), 3);
    TS_ASSERT_EQUALS(output->columnCount(), 2);
    auto column = output->getColumn("DataColumn");
    TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(1), 29.3261, 0.00005);
    TS_ASSERT_DELTA(column->toDouble(2), 44.3412, 0.00005);
    column = output->getColumn("DataColumn_1");
    TS_ASSERT_DELTA(column->toDouble(0), 2.74937 * c_mbsr, 0.000005 * c_mbsr);
    TS_ASSERT_DELTA(column->toDouble(1), 0.7204 * c_mbsr, 0.00005 * c_mbsr);
    TS_ASSERT_DELTA(column->toDouble(2), 0.429809 * c_mbsr, 0.0000005 * c_mbsr);

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
    fun->setAttributeValue("ToleranceIntensity", 0.001 * c_mbsr);

    auto data = TableWorkspace_sptr(new TableWorkspace);
    data->addColumn("double", "Energy");
    data->addColumn("double", "Intensity");

    TableRow row = data->appendRow();
    row << 0.0 << 2.74937 * c_mbsr;
    row = data->appendRow();
    row << 29.3261 << 0.7204 * c_mbsr;
    row = data->appendRow();
    row << 44.3412 << 0.429809 * c_mbsr;

    EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun);
    eval.setProperty("InputWorkspace", data);
    eval.setProperty("DataColumn", "Energy");
    eval.setProperty("DataColumn_1", "Intensity");
    eval.setProperty("OutputWorkspace", "out");
    eval.execute();

    ITableWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("out");
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->rowCount(), 3);
    TS_ASSERT_EQUALS(output->columnCount(), 4);
    auto column = output->getColumn("Energy");
    TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(1), 29.3261, 0.00005);
    TS_ASSERT_DELTA(column->toDouble(2), 44.3412, 0.00005);
    column = output->getColumn("Intensity");
    TS_ASSERT_DELTA(column->toDouble(0), 2.74937 * c_mbsr, 0.000005 * c_mbsr);
    TS_ASSERT_DELTA(column->toDouble(1), 0.7204 * c_mbsr, 0.00005 * c_mbsr);
    TS_ASSERT_DELTA(column->toDouble(2), 0.429809 * c_mbsr, 0.0000005 * c_mbsr);
    column = output->getColumn("Energy_calc");
    TS_ASSERT_DELTA(column->toDouble(0), 0.0, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(1), 28.7149, 0.0001);
    TS_ASSERT_DELTA(column->toDouble(2), 43.3162, 0.0001);
    column = output->getColumn("Intensity_calc");
    TS_ASSERT_DELTA(column->toDouble(0), 2.7483 * c_mbsr, 0.0001 * c_mbsr);
    TS_ASSERT_DELTA(column->toDouble(1), 0.7394 * c_mbsr, 0.0001 * c_mbsr);
    TS_ASSERT_DELTA(column->toDouble(2), 0.4116 * c_mbsr, 0.0001 * c_mbsr);

    AnalysisDataService::Instance().clear();
  }

  void test_symmetry() {
    CrystalFieldPeaks fun;
    fun.setAttributeValue("Ion", "Ce");
    TS_ASSERT(!isFixed(fun, "B20"));
    TS_ASSERT(!isFixed(fun, "B21"));
    TS_ASSERT(!isFixed(fun, "IB21"));
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
    TS_ASSERT(!fun.isActive(i));
    TS_ASSERT(isFixed(fun, "IB44"));

    TS_ASSERT(!isFixed(fun, "B60"));
    TS_ASSERT(isFixed(fun, "B61"));
    TS_ASSERT(isFixed(fun, "IB61"));
    TS_ASSERT(isFixed(fun, "B62"));
    TS_ASSERT(isFixed(fun, "IB62"));
    TS_ASSERT(isFixed(fun, "B63"));
    TS_ASSERT(isFixed(fun, "IB63"));
    i = fun.parameterIndex("B64");
    TS_ASSERT(!fun.isActive(i));
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

  void test_CrystalFieldPeaksBaseImpl() { Mantid::CurveFitting::Functions::CrystalFieldPeaksBaseImpl fun; }

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
