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
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
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
    ITableWorkspace_sptr calTableWs =
        WorkspaceFactory::Instance().createTable();
    calTableWs->addColumn("int", "Detector ID");
    calTableWs->addColumn("V3D", "Detector Position");
    calTableWs->addColumn("double", "Detector Y Coordinate");
    calTableWs->addColumn("double", "Detector Height");
    calTableWs->addColumn("double", "Detector Width");

    for (int i = 0; i < ndets; ++i) {
      TableRow row = calTableWs->appendRow();
      //  detector-ID  position  Y-coordinate  Height  Width
      row << i + 1 << V3D(1.0, 0.01 * i, 2.0) << 0.04 * i << 0.04 << 0.05;
    }
    TS_ASSERT_THROWS_NOTHING(appCalib.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(appCalib.setProperty<ITableWorkspace_sptr>(
        "CalibrationTable", calTableWs));
    TS_ASSERT_THROWS_NOTHING(appCalib.execute());

    TS_ASSERT(appCalib.isExecuted());

    const auto &spectrumInfo = ws->spectrumInfo();
    const auto &componentInfo = ws->componentInfo();

    int id = spectrumInfo.detector(0).getID();
    V3D newPos = spectrumInfo.position(0);
    V3D scaleFactor = componentInfo.scaleFactor(0);

    TS_ASSERT_EQUALS(id, 1);
    TS_ASSERT_DELTA(newPos.X(), 1.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Z(), 2.0, 0.0001);
    TS_ASSERT_DELTA(scaleFactor.Y(), 2.0, 0.0001); // original height was 0.02
    TS_ASSERT_DELTA(scaleFactor.X(), 0.5, 0.0001); // original width was 0.1

    id = spectrumInfo.detector(ndets - 1).getID();
    newPos = spectrumInfo.position(ndets - 1);
    scaleFactor = componentInfo.scaleFactor(0);

    TS_ASSERT_EQUALS(id, ndets);
    TS_ASSERT_DELTA(newPos.X(), 1.0, 0.0001);
    TS_ASSERT_DELTA(newPos.Y(), 0.04 * (ndets - 1), 0.0001);
    TS_ASSERT_DELTA(newPos.Z(), 2.0, 0.0001);
    TS_ASSERT_DELTA(scaleFactor.Y(), 2.0, 0.0001);
    TS_ASSERT_DELTA(scaleFactor.X(), 0.5, 0.0001);

    dataStore.remove(wsName);
  }

  /**
   * Load a *.raw file and reset the detector position, width, and height for
   * the first two spectra
   */
  void testCalibrateRawFile() {
    // Create a calibration table
    ITableWorkspace_sptr calTableWs =
        WorkspaceFactory::Instance().createTable();
    calTableWs->addColumn("int", "Detector ID");
    calTableWs->addColumn("V3D", "Detector Position");
    calTableWs->addColumn("double", "Detector Y Coordinate");
    calTableWs->addColumn("double", "Detector Width");
    calTableWs->addColumn("double", "Detector Height");

    // Load the first two spectra from a *.raw data file into a workspace
    const int nSpectra = 2;
    const std::string wsName("applyCalibrationToRaw");
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "HRP39180.RAW");
    loader.setPropertyValue("OutputWorkspace", wsName);
    loader.setPropertyValue("SpectrumMin",
                            "1"); // Spectrum number, not workspace index
    loader.setPropertyValue("SpectrumMax", "9"); // std::to_string(nSpectra));
    loader.execute();
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    MatrixWorkspace_sptr workspace =
        dataStore.retrieveWS<MatrixWorkspace>(wsName);
    const auto &detectorInfo = workspace->detectorInfo();
    const auto &componentInfo = workspace->componentInfo();

    // Populate the calibration table with some final detector positions,
    // widths, and heights
    const std::vector<V3D> positions{V3D(0.20, 0.0, 0.42),
                                     V3D(0.53, 0.0, 0.75)};
    const std::vector<double> yCoords{0.31, 0.64};
    const std::vector<double> widths{0.008, 0.007};
    const std::vector<double> heights{0.041, 0.039};
    for (size_t i = 0; i < nSpectra; i++) {
      IDetector_const_sptr detector = workspace->getDetector(i);
      const auto detectorID = detector->getID();
      TableRow row = calTableWs->appendRow();
      // insert data in the same order in which table columns were declared
      // detector-ID  position  Y-coordinate  Width Height
      row << detectorID << positions[i] << yCoords[i] << widths[i]
          << heights[i];
    }

    // Apply the calibration to the workspace
    ApplyCalibration calibrationAlgorithm;
    calibrationAlgorithm.initialize();
    TS_ASSERT(calibrationAlgorithm.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        calibrationAlgorithm.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        calibrationAlgorithm.setProperty<ITableWorkspace_sptr>(
            "CalibrationTable", calTableWs));
    TS_ASSERT_THROWS_NOTHING(calibrationAlgorithm.execute());
    TS_ASSERT(calibrationAlgorithm.isExecuted());

    // Assert the calibration
    for (size_t i = 0; i < nSpectra; i++) {
      // assert detector position
      IDetector_const_sptr detector = workspace->getDetector(i);
      const auto detectorID = detector->getID();
      const auto &newPosition = detector->getPos();
      TS_ASSERT_DELTA(newPosition.X(), positions[i].X(), 0.0001);
      TS_ASSERT_DELTA(newPosition.Y(), yCoords[i], 0.0001);
      TS_ASSERT_DELTA(newPosition.Z(), positions[i].Z(), 0.0001);
      // assert detector width and height
      const auto detectorIndex = detectorInfo.indexOf(detectorID);
      const auto &scaleFactor = componentInfo.scaleFactor(detectorIndex);
      const auto &box =
          componentInfo.shape(detectorIndex).getBoundingBox().width();
      TS_ASSERT_DELTA(scaleFactor.X() * box.X(), widths[i], 0.0001);
      TS_ASSERT_DELTA(scaleFactor.Y() * box.Y(), heights[i], 0.0001);
    }
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
    ITableWorkspace_sptr calTableWs =
        WorkspaceFactory::Instance().createTable();
    calTableWs->addColumn("int", "Detector ID");
    calTableWs->addColumn("V3D", "Detector Position");

    for (int i = 0; i < ndets; ++i) {
      TableRow row = calTableWs->appendRow();
      //               detector ID
      row << firstDetectorID + 10 * i << V3D(1.0, 0.01 * i, 2.0);
    }
    TS_ASSERT_THROWS_NOTHING(appCalib.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(appCalib.setProperty<ITableWorkspace_sptr>(
        "CalibrationTable", calTableWs));
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
