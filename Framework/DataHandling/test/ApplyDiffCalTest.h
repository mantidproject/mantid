// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/ApplyDiffCal.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

namespace {
const size_t NUM_BANK = 5;
} // namespace

class ApplyDiffCalTest : public CxxTest::TestSuite {
public:
  void testName() {
    ApplyDiffCal appDiffCal;
    TS_ASSERT_EQUALS(appDiffCal.name(), "ApplyDiffCal")
  }

  void testInit() {
    ApplyDiffCal appDiffCal;
    appDiffCal.initialize();
    TS_ASSERT(appDiffCal.isInitialized())
  }

  void testExec() {
    auto calWSIn = createCalibration(5 * 9); // nine components per bank

    Mantid::Geometry::Instrument_sptr inst = createInstrument();

    std::string testWorkspaceName = "TestApplyDiffCalWorkspace";
    AnalysisDataService::Instance().add(testWorkspaceName, std::make_shared<Workspace2D>());
    auto instrumentWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(testWorkspaceName);
    instrumentWS->setInstrument(inst);

    ApplyDiffCal appDiffCal;
    TS_ASSERT_THROWS_NOTHING(appDiffCal.initialize());
    TS_ASSERT(appDiffCal.isInitialized());
    TS_ASSERT_THROWS_NOTHING(appDiffCal.setProperty("InstrumentWorkspace", testWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(appDiffCal.setProperty("CalibrationWorkspace", calWSIn));
    TS_ASSERT_THROWS_NOTHING(appDiffCal.execute(););
    TS_ASSERT(appDiffCal.isExecuted());

    auto instFromWS = instrumentWS->getInstrument();
    auto det = instFromWS->getDetector(3);
    auto pmap = instFromWS->getParameterMap();
    auto par = pmap->getRecursive(det.get(), "DIFC");
    double difc{0.};
    if (par)
      difc = par->value<double>();
    TS_ASSERT_EQUALS(difc, 102);
    par = pmap->getRecursive(det.get(), "DIFA");
    double difa{0.};
    if (par)
      difa = par->value<double>();
    TS_ASSERT_EQUALS(difa, 4);
    par = pmap->getRecursive(det.get(), "TZERO");
    double tzero{0.};
    if (par)
      tzero = par->value<double>();
    TS_ASSERT_EQUALS(tzero, 2);

    ApplyDiffCal appDiffCalClear;
    pmap->addDouble(det->getComponentID(), "extraparam", 1.23);
    TS_ASSERT_THROWS_NOTHING(appDiffCalClear.initialize());
    TS_ASSERT(appDiffCalClear.isInitialized());
    TS_ASSERT_THROWS_NOTHING(appDiffCalClear.setProperty("InstrumentWorkspace", testWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(appDiffCalClear.setProperty("ClearCalibration", true));
    TS_ASSERT_THROWS_NOTHING(appDiffCalClear.execute(););
    TS_ASSERT(appDiffCalClear.isExecuted());

    instFromWS = instrumentWS->getInstrument();
    det = instFromWS->getDetector(3);
    pmap = instFromWS->getParameterMap();
    par = pmap->getRecursive(det.get(), "DIFC");
    TS_ASSERT(!par);
    par = pmap->getRecursive(det.get(), "extraparam");
    TS_ASSERT(par);
  }

  void TestClear() {}

private:
  Mantid::Geometry::Instrument_sptr createInstrument() {
    auto instr = ComponentCreationHelper::createTestInstrumentCylindrical(NUM_BANK);
    auto pmap = std::make_shared<Mantid::Geometry::ParameterMap>();
    instr = std::make_shared<Mantid::Geometry::Instrument>(instr, pmap);
    return instr;
  }

  TableWorkspace_sptr createCalibration(const size_t numRows) {
    TableWorkspace_sptr wksp = std::make_shared<TableWorkspace>();
    wksp->addColumn("int", "detid");
    wksp->addColumn("double", "difc");
    wksp->addColumn("double", "difa");
    wksp->addColumn("double", "tzero");
    wksp->addColumn("double", "tofmin");

    for (size_t i = 0; i < numRows; ++i) {
      TableRow row = wksp->appendRow();

      row << static_cast<int>(i + 1)      // detid
          << static_cast<double>(100 + i) // difc
          << static_cast<double>(i * i)   // difa
          << static_cast<double>(i)       // tzero
          << 0.;                          // tofmin
    }
    return wksp;
  }
};
