// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef APPLYCALIBRATIONTEST_H_
#define APPLYCALIBRATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAlgorithms/ApplyCalibration.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IDetector_const_sptr;

class ApplyCalibrationTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(appCalib.name(), "ApplyCalibration") }

  void testInit() {
    appCalib.initialize();
    TS_ASSERT(appCalib.isInitialized())
  }

  void testSimple() {

    int ndets = 3;

    // Create workspace with paremeterised instrument and put into data store
    Workspace2D_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(ndets, 10,
                                                                     true);
    const std::string wsName("ApplyCabrationWs");
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    dataStore.add(wsName, ws);

    // Create Calibration Table
    ITableWorkspace_sptr posTableWs =
        WorkspaceFactory::Instance().createTable();
    posTableWs->addColumn("int", "Detector ID");
    posTableWs->addColumn("V3D", "Detector Position");

    for (int i = 0; i < ndets; ++i) {
      TableRow row = posTableWs->appendRow();
      row << i + 1 << V3D(1.0, 0.01 * i, 2.0);
    }
    TS_ASSERT_THROWS_NOTHING(appCalib.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(appCalib.setProperty<ITableWorkspace_sptr>(
        "PositionTable", posTableWs));
    TS_ASSERT_THROWS_NOTHING(appCalib.execute());

    TS_ASSERT(appCalib.isExecuted());

    const auto &spectrumInfo = ws->spectrumInfo();

    int id = spectrumInfo.detector(0).getID();
    V3D newPos = spectrumInfo.position(0);
    TS_ASSERT_EQUALS(id, 1);
    TS_ASSERT_DELTA(newPos.X(), 1.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Z(), 2.0, 0.0001);

    id = spectrumInfo.detector(ndets - 1).getID();
    newPos = spectrumInfo.position(ndets - 1);
    TS_ASSERT_EQUALS(id, ndets);
    TS_ASSERT_DELTA(newPos.X(), 1.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Y(), 0.01 * (ndets - 1), 0.0001);
    TS_ASSERT_DELTA(newPos.Z(), 2.0, 0.0001);

    dataStore.remove(wsName);
  }

  void testComplex() {
    /* The purpse of this test is to test the algorithm when the relative
     * positioning and rotation
     * of components is complicated. This is the case for the MAPS instrument
     * and so here we
     * load the IDF of a MAPS instrument where the number of detectors has been
     * reduced.
     */

    int ndets = 3;

    // Create workspace with a reduced MAPS instrument (parametrised) and
    // retrieve vfrom data store.
    const std::string wsName("ApplyCabrationWs");
    Mantid::DataHandling::LoadEmptyInstrument loader;
    loader.initialize();
    loader.setPropertyValue("Filename",
                            "unit_testing/MAPS_Definition_Reduced.xml");
    loader.setPropertyValue("OutputWorkspace", wsName);
    loader.execute();
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    MatrixWorkspace_sptr ws = dataStore.retrieveWS<MatrixWorkspace>(wsName);

    // Create Calibration Table
    int firstDetectorID = 34208002;
    ITableWorkspace_sptr posTableWs =
        WorkspaceFactory::Instance().createTable();
    posTableWs->addColumn("int", "Detector ID");
    posTableWs->addColumn("V3D", "Detector Position");

    for (int i = 0; i < ndets; ++i) {
      TableRow row = posTableWs->appendRow();
      row << firstDetectorID + 10 * i << V3D(1.0, 0.01 * i, 2.0);
    }
    TS_ASSERT_THROWS_NOTHING(appCalib.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(appCalib.setProperty<ITableWorkspace_sptr>(
        "PositionTable", posTableWs));
    TS_ASSERT_THROWS_NOTHING(appCalib.execute());

    TS_ASSERT(appCalib.isExecuted());

    IDetector_const_sptr det = ws->getDetector(1830);
    int id = det->getID();
    V3D newPos = det->getPos();
    TS_ASSERT_EQUALS(id, firstDetectorID);
    TS_ASSERT_DELTA(newPos.X(), 1.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Z(), 2.0, 0.0001);

    det = ws->getDetector(1840);
    id = det->getID();
    newPos = det->getPos();
    TS_ASSERT_EQUALS(id, firstDetectorID + 10);
    TS_ASSERT_DELTA(newPos.X(), 1.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Y(), 0.01, 0.0001);
    TS_ASSERT_DELTA(newPos.Z(), 2.0, 0.0001);

    det = ws->getDetector(1850);
    id = det->getID();
    newPos = det->getPos();
    TS_ASSERT_EQUALS(id, firstDetectorID + 20);
    TS_ASSERT_DELTA(newPos.X(), 1.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Y(), 0.02, 0.0001);
    TS_ASSERT_DELTA(newPos.Z(), 2.0, 0.0001);

    dataStore.remove(wsName);
  }

private:
  ApplyCalibration appCalib;
};

#endif /*APPLYCALIBRATIONTEST_H_*/
