// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CorelliCalibrationApply.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Logger.h"

#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

///
/// The base class CxxTest doc is available at
/// http://cxxtest.com/guide.html
class CorelliCalibrationApplyTest : public CxxTest::TestSuite {
public:
  void testName() {
    CorelliCalibrationApply corelliPCA;
    TS_ASSERT_EQUALS(corelliPCA.name(), "CorelliCalibrationApply");
  }

  void testInit() {
    CorelliCalibrationApply crlCalApp;
    crlCalApp.initialize();
    TS_ASSERT(crlCalApp.isInitialized());
  }

  void testValidateWS() {
    // get a mock workspace with wrong instrument name
    IAlgorithm_sptr lei = AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    lei->initialize();
    lei->setPropertyValue("Filename", "NOW4_Definition.xml");
    lei->setPropertyValue("OutputWorkspace", "wrongTypeWs");
    lei->setPropertyValue("MakeEventWorkspace", "1");
    lei->execute();
    auto ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("wrongTypeWs");

    // get a mock calTable
    std::string calTableName = "correctCalTable";
    auto calTable = createTestCalibrationTableWorkspace(calTableName);

    // setup alg
    CorelliCalibrationApply alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", "wrongTypeWs");
    alg.setPropertyValue("CalibrationTable", calTableName);

    // ensure that an exception is thrown here
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void testValidateCalTable() {
    // get a mock workspace with correct instrument name
    auto ws = createTestEventWorkspace();

    // get a mock calTable with wrong header
    std::string calTableName = "wrongCalTable";
    auto calTable = createTestCalibrationTableWorkspace(calTableName);
    calTable->removeColumn("Xposition");

    // setup alg
    CorelliCalibrationApply alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", "correctWs");
    alg.setPropertyValue("CalibrationTable", calTableName);

    // ensure that an exception is thrown here
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void testExec() {
    // setup input workspace
    auto ws = createTestEventWorkspace();

    // get a correct mock calibration table
    std::string calTableName = "correctCalTable";
    auto calTable = createTestCalibrationTableWorkspace(calTableName);

    // setup alg
    CorelliCalibrationApply alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", "correctWs");
    alg.setPropertyValue("CalibrationTable", calTableName);

    // make sure no exception is thrown here
    TS_ASSERT_THROWS_NOTHING(alg.execute())
  }

private:
  EventWorkspace_sptr createTestEventWorkspace() {
    // Name of the output workspace.
    std::string outWSName("correctWs");

    IAlgorithm_sptr lei = AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    lei->initialize();
    lei->setPropertyValue("Filename", "CORELLI_Definition.xml");
    lei->setPropertyValue("OutputWorkspace", outWSName);
    lei->setPropertyValue("MakeEventWorkspace", "1");
    lei->execute();

    EventWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName);
    return ws;
  }

  TableWorkspace_sptr createTestCalibrationTableWorkspace(const std::string &outWSName) {

    ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(outWSName, itablews);

    TableWorkspace_sptr tablews = std::dynamic_pointer_cast<TableWorkspace>(itablews);
    TS_ASSERT(tablews);

    // Set up columns
    for (size_t i = 0; i < CorelliCalibration::calibrationTableColumnNames.size(); i++) {
      std::string colname = CorelliCalibration::calibrationTableColumnNames[i];
      std::string type = CorelliCalibration::calibrationTableColumnTypes[i];
      tablews->addColumn(type, colname);
    }

    // append rows
    Mantid::API::TableRow sourceRow = tablews->appendRow();
    sourceRow << "moderator" << 0. << 0. << -15.560 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow sampleRow = tablews->appendRow();
    sampleRow << "sample-position" << 0.0001 << -0.0002 << 0.003 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow bank1Row = tablews->appendRow();
    bank1Row << "bank1" << 0.9678 << 0.0056 << 0.0003 << 0.4563 << -0.9999 << 0.3424 << 5.67;
    // bank42 is at the x-axis (transverse of beam direction)
    // rotating 180 should reverse its bottom pixel (1) and top pixel (256)
    TableRow bank42Row = tablews->appendRow();
    bank42Row << "bank42" << 0. << 0. << 0. << 1. << 0. << 0. << 180.;

    return tablews;
  }
};
