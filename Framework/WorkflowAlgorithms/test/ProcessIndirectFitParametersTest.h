// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERSTEST_H_
#define MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidWorkflowAlgorithms/ProcessIndirectFitParameters.h"

using Mantid::Algorithms::ProcessIndirectFitParameters;
using namespace Mantid::API;

class ProcessIndirectFitParametersTest : public CxxTest::TestSuite {

private:
  ITableWorkspace_sptr createTable() {
    auto tableWs = WorkspaceFactory::Instance().createTable();
    tableWs->addColumn("double", "axis-1");
    tableWs->addColumn("double", "f0.A0");
    tableWs->addColumn("double", "f0.A0_Err");
    tableWs->addColumn("double", "f1.f1.f0.Height");
    tableWs->addColumn("double", "f1.f1.f0.Height_Err");
    tableWs->addColumn("double", "f1.f1.f0.Amplitude");
    tableWs->addColumn("double", "f1.f1.f0.Amplitude_Err");
    tableWs->addColumn("double", "f1.f1.f0.PeakCentre");
    tableWs->addColumn("double", "f1.f1.f0.PeakCentre_Err");

    size_t n = 5;
    for (size_t i = 0; i < n; ++i) {
      TableRow row = tableWs->appendRow();
      double ax1 = int(i) * 1.0;
      double a0 = 0.0;
      double a0e = 0.0;
      double h = int(i) * 1.02;
      double he = sqrt(h);
      double am = int(i) * 2.43;
      double ame = sqrt(am);
      double pc = -0.0567;
      double pce = sqrt(pc);
      row << ax1 << a0 << a0e << h << he << am << ame << pc << pce;
    }
    return tableWs;
  }

  ITableWorkspace_sptr createIrregularTable() {
    auto tableWs = WorkspaceFactory::Instance().createTable();
    tableWs->addColumn("double", "axis-1");
    tableWs->addColumn("double", "f1.f1.f0.Height");
    tableWs->addColumn("double", "f1.f1.f0.Height_Err");
    tableWs->addColumn("double", "f1.f1.f0.Amplitude");
    tableWs->addColumn("double", "f1.f1.f0.Amplitude_Err");
    tableWs->addColumn("double", "f1.f1.f1.Height");
    tableWs->addColumn("double", "f1.f1.f1.Height_Err");
    tableWs->addColumn("double", "f1.f1.f2.Height");
    tableWs->addColumn("double", "f1.f1.f2.Height_Err");

    size_t n = 5;
    for (size_t i = 0; i < n; ++i) {
      TableRow row = tableWs->appendRow();
      double ax1 = int(i) * 1.0;
      double h = int(i) * 1.02;
      double he = sqrt(h);
      double am = int(i) * 2.43;
      double ame = sqrt(am);
      double h1 = -0.0567;
      double he1 = sqrt(h);
      double h2 = int(i) * -0.25;
      double he2 = sqrt(h2);
      row << ax1 << h << he << am << ame << h1 << he1 << h2 << he2;
    }
    return tableWs;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessIndirectFitParametersTest *createSuite() {
    return new ProcessIndirectFitParametersTest();
  }
  static void destroySuite(ProcessIndirectFitParametersTest *suite) {
    delete suite;
  }

  void test_empty_input_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("InputWorkspace", ""),
                     const std::invalid_argument &);
  }

  void test_empty_x_column_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("ColumnX", ""),
                     const std::invalid_argument &);
  }

  void test_empty_param_names_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("ParameterNames", ""),
                     const std::invalid_argument &);
  }

  void test_empty_output_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("OutputWorkspace", ""),
                     const std::invalid_argument &);
  }

  void test_property_input() {
    auto tableWs = createTable();
    std::string xColumn = "axis-1";
    std::string parameterValues = "Amplitude";
    std::string inAxis = "Degrees";
    std::string outputName = "outMatrix";

    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace", tableWs);
    alg.setPropertyValue("ColumnX", xColumn);
    alg.setPropertyValue("ParameterNames", parameterValues);
    alg.setPropertyValue("XAxisUnit", inAxis);
    alg.setProperty("OutputWorkspace", outputName);

    ITableWorkspace_sptr tableProp = alg.getProperty("InputWorkspace");

    TS_ASSERT_EQUALS(tableProp, tableWs);
    TS_ASSERT_EQUALS(std::string(alg.getProperty("ColumnX")), xColumn);
    TS_ASSERT_EQUALS(std::string(alg.getProperty("ParameterNames")),
                     parameterValues);
    TS_ASSERT_EQUALS(std::string(alg.getProperty("XAxisUnit")), inAxis);
    TS_ASSERT_EQUALS(std::string(alg.getProperty("OutputWorkspace")),
                     outputName);
  }

  void test_output_of_regular_shaped_table_workspace() {
    auto tableWs = createTable();
    std::string xColumn = "axis-1";
    std::string parameterValues = "Height,Amplitude";
    std::string inAxis = "Degrees";
    std::string outputName = "outMatrix";

    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", tableWs);
    alg.setPropertyValue("ColumnX", xColumn);
    alg.setPropertyValue("ParameterNames", parameterValues);
    alg.setPropertyValue("XAxisUnit", inAxis);
    alg.setProperty("OutputWorkspace", outputName);

    alg.execute();

    MatrixWorkspace_sptr outWs;
    TS_ASSERT_THROWS_NOTHING(
        outWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputName));
    size_t const histNum = outWs->getNumberHistograms();
    TS_ASSERT_EQUALS(histNum, 2);
    TS_ASSERT_EQUALS(outWs->getAxis(1)->label(0), "f1.f1.f0.Height");
    TS_ASSERT_EQUALS(outWs->getAxis(1)->label(1), "f1.f1.f0.Amplitude");

    // 5 = The initial number of rows in the table workspace
    TS_ASSERT_EQUALS(outWs->blocksize(), 5);

    // Test output values
    auto heightY = outWs->readY(0);
    auto heightTest = tableWs->getColumn("f1.f1.f0.Height")->numeric_fill<>();
    TS_ASSERT_EQUALS(heightY, heightTest);

    auto ampY = outWs->readY(1);
    auto ampTest = tableWs->getColumn("f1.f1.f0.Amplitude")->numeric_fill<>();
    TS_ASSERT_EQUALS(ampY, ampTest);

    // Test axis units
    std::string outAxis = outWs->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(inAxis, outAxis);

    AnalysisDataService::Instance().remove(outputName);
  }

  void test_output_of_irregular_shaped_table_workspace() {
    auto tableWs = createIrregularTable();
    std::string xColumn = "axis-1";
    std::string parameterValues = "Height,Amplitude";
    std::string inAxis = "Degrees";
    std::string outputName = "outMatrix";

    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", tableWs);
    alg.setPropertyValue("ColumnX", xColumn);
    alg.setPropertyValue("ParameterNames", parameterValues);
    alg.setPropertyValue("XAxisUnit", inAxis);
    alg.setProperty("OutputWorkspace", outputName);

    alg.execute();

    MatrixWorkspace_sptr outWs;
    TS_ASSERT_THROWS_NOTHING(
        outWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputName));
    size_t const histNum = outWs->getNumberHistograms();
    TS_ASSERT_EQUALS(histNum, 4);
    TS_ASSERT_EQUALS(outWs->getAxis(1)->label(0), "f1.f1.f0.Height");
    TS_ASSERT_EQUALS(outWs->getAxis(1)->label(1), "f1.f1.f0.Amplitude");
    TS_ASSERT_EQUALS(outWs->getAxis(1)->label(2), "f1.f1.f1.Height");
    TS_ASSERT_EQUALS(outWs->getAxis(1)->label(3), "f1.f1.f2.Height");

    // 5 = The initial number of rows in the table workspace
    TS_ASSERT_EQUALS(outWs->blocksize(), 5);

    // Test output values
    auto heightY = outWs->readY(0);
    auto heightTest = tableWs->getColumn("f1.f1.f0.Height")->numeric_fill<>();
    TS_ASSERT_EQUALS(heightY, heightTest);

    auto ampY = outWs->readY(1);
    auto ampTest = tableWs->getColumn("f1.f1.f0.Amplitude")->numeric_fill<>();
    TS_ASSERT_EQUALS(ampY, ampTest);

    auto height1Y = outWs->readY(2);
    auto height1Test = tableWs->getColumn("f1.f1.f1.Height")->numeric_fill<>();
    TS_ASSERT_EQUALS(height1Y, height1Test);

    auto height2Y = outWs->readY(3);
    auto height2Test = tableWs->getColumn("f1.f1.f2.Height")->numeric_fill<>();
    TS_ASSERT_EQUALS(height2Y, height2Test);

    // Test axis units
    std::string outAxis = outWs->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(inAxis, outAxis);

    AnalysisDataService::Instance().remove(outputName);
  }
};

#endif /* MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERSTEST_H_ */
