// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLETEST_H_
#define MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvertUnitsUsingDetectorTable.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/UnitFactory.h"

using Mantid::Algorithms::ConvertUnitsUsingDetectorTable;
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class ConvertUnitsUsingDetectorTableTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertUnitsUsingDetectorTableTest *createSuite() {
    return new ConvertUnitsUsingDetectorTableTest();
  }
  static void destroySuite(ConvertUnitsUsingDetectorTableTest *suite) {
    delete suite;
  }
  void test_Init() {
    ConvertUnitsUsingDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_TofToLambda() {
    ConvertUnitsUsingDetectorTable myAlg;
    myAlg.initialize();
    TS_ASSERT(myAlg.isInitialized());

    const std::string workspaceName("_ws_testConvertUsingDetectorTable");
    int nBins = 10;
    //     MatrixWorkspace_sptr WS =
    //     WorkspaceCreationHelper::Create2DWorkspaceBinned(2, nBins, 500.0,
    //     50.0);
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            2, nBins, false, false, true, "TESTY");

    WS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    AnalysisDataService::Instance().add(workspaceName, WS);

    // Create TableWorkspace with values in it

    ITableWorkspace_sptr pars =
        WorkspaceFactory::Instance().createTable("TableWorkspace");
    pars->addColumn("int", "spectra");
    pars->addColumn("double", "l1");
    pars->addColumn("double", "l2");
    pars->addColumn("double", "twotheta");
    pars->addColumn("double", "efixed");
    pars->addColumn("int", "emode");

    API::TableRow row0 = pars->appendRow();
    row0 << 1 << 100.0 << 10.0 << 90.0 << 7.0 << 0;

    API::TableRow row1 = pars->appendRow();
    row1 << 2 << 1.0 << 1.0 << 90.0 << 7.0 << 0;

    // Set the properties
    myAlg.setRethrows(true);
    myAlg.setPropertyValue("InputWorkspace", workspaceName);
    myAlg.setPropertyValue("OutputWorkspace", workspaceName);
    myAlg.setPropertyValue("Target", "Wavelength");
    myAlg.setProperty("DetectorParameters", pars);
    myAlg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        workspaceName);

    TS_ASSERT_DELTA(outWS->x(0)[0], 0.0, 0.000001);
    TS_ASSERT_DELTA(outWS->x(0)[9], 0.000323676, 0.000001);

    AnalysisDataService::Instance().remove(workspaceName);
  };

  void testDeltaEFailDoesNotAlterInPlaceWorkspace() {

    std::string wsName = "ConvertUnitsUsingDetectorTable_"
                         "testDeltaEFailDoesNotAlterInPlaceWorkspace";
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 200,
                                                                     false);
    // set to a distribution
    ws->setDistribution(true);
    AnalysisDataService::Instance().add(wsName, ws);

    // get a copy of some original values
    auto originalUnit = ws->getAxis(0)->unit();
    auto originalEMode = ws->getEMode();
    TS_ASSERT_THROWS_ANYTHING(ws->getEFixed());
    auto originalYdata = ws->readY(0);

    // Create TableWorkspace with values in it
    ITableWorkspace_sptr pars =
        WorkspaceFactory::Instance().createTable("TableWorkspace");
    pars->addColumn("int", "spectra");
    pars->addColumn("double", "l1");
    pars->addColumn("double", "l2");
    pars->addColumn("double", "twotheta");
    pars->addColumn("double", "efixed");
    pars->addColumn("int", "emode");

    // do not set emode to a valid number - this will cause a failure
    // do not set efixed either
    API::TableRow row0 = pars->appendRow();
    row0 << 1 << 100.0 << 10.0 << 90.0 << 7.0 << 0;

    API::TableRow row1 = pars->appendRow();
    row1 << 2 << 1.0 << 1.0 << 90.0 << 7.0 << 0;

    ConvertUnitsUsingDetectorTable conv;
    conv.initialize();
    conv.setPropertyValue("InputWorkspace", wsName);
    // in place conversion
    conv.setPropertyValue("OutputWorkspace", wsName);
    conv.setPropertyValue("Target", "DeltaE");
    conv.setProperty("DetectorParameters", pars);

    conv.execute();

    TSM_ASSERT("Expected ConvertUnitsUsingDetectorTable to throw on deltaE "
               "conversion without valid"
               "eMode",
               !conv.isExecuted());

    TS_ASSERT_EQUALS(originalUnit, ws->getAxis(0)->unit());
    TS_ASSERT_EQUALS(originalEMode, ws->getEMode());
    TS_ASSERT_THROWS_ANYTHING(ws->getEFixed());
    TS_ASSERT_EQUALS(originalYdata, ws->readY(0));

    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif /* MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLETEST_H_ */
