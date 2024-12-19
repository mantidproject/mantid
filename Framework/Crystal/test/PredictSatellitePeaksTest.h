// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * PredictSatellitePeaksTest.h
 *
 *  Created on: July, 2018
 *      Author: Vickie Lynch
 */

#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidCrystal/CalculateUMatrix.h"
#include "MantidCrystal/FindUBUsingIndexedPeaks.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/PredictSatellitePeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include <iostream>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class PredictSatellitePeaksTest : public CxxTest::TestSuite {

public:
  void test_Init() {
    PredictSatellitePeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec() {
    LoadIsawPeaks alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize())
    TS_ASSERT(alg1.isInitialized())
    alg1.setPropertyValue("Filename", "Modulated.peaks");
    alg1.setPropertyValue("OutputWorkspace", "Modulated");

    TS_ASSERT(alg1.execute());
    TS_ASSERT(alg1.isExecuted());

    IPeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("Modulated")));
    TS_ASSERT(ws);
    if (!ws)
      return;

    FindUBUsingIndexedPeaks alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("ToleranceForSatellite", "0.05"));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("PeaksWorkspace", "Modulated"));
    TS_ASSERT_THROWS_NOTHING(alg2.execute(););
    TS_ASSERT(alg2.isExecuted());

    PredictSatellitePeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("Peaks", "Modulated");
    alg.setProperty("SatellitePeaks", std::string("SatellitePeaks"));
    alg.setProperty("MaxOrder", "1");
    alg.setProperty("GetModVectorsFromUB", true);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());
    alg.setPropertyValue("SatellitePeaks", "SatellitePeaks");
    IPeaksWorkspace_sptr SatellitePeaks = alg.getProperty("SatellitePeaks");
    TS_ASSERT(SatellitePeaks);

    TS_ASSERT_EQUALS(SatellitePeaks->getNumberPeaks(), 40);

    auto &peak0 = SatellitePeaks->getPeak(4);
    TS_ASSERT_DELTA(peak0.getH(), 1.49, .01);
    TS_ASSERT_DELTA(peak0.getK(), -0.56, .01);
    TS_ASSERT_DELTA(peak0.getL(), 1.61, .01);

    auto &peak3 = SatellitePeaks->getPeak(6);
    TS_ASSERT_DELTA(peak3.getH(), 1.51, .01);
    TS_ASSERT_DELTA(peak3.getK(), -0.44, .01);
    TS_ASSERT_DELTA(peak3.getL(), 1.39, .01);

    PredictSatellitePeaks alg4;
    TS_ASSERT_THROWS_NOTHING(alg4.initialize());
    TS_ASSERT(alg4.isInitialized());
    alg4.setProperty("Peaks", "Modulated");
    alg4.setProperty("SatellitePeaks", std::string("SatellitePeaks"));
    alg4.setProperty("MaxOrder", "1");
    alg4.setProperty("IncludeAllPeaksInRange", true);
    alg4.setProperty("GetModVectorsFromUB", true);
    alg4.setProperty("MinDSpacing", "0.5");
    alg4.setProperty("MaxDSpacing", "3");
    alg4.setProperty("WavelengthMin", "1");
    alg4.setProperty("WavelengthMax", "2");
    TS_ASSERT(alg4.execute())
    TS_ASSERT(alg4.isExecuted());
    alg4.setPropertyValue("SatellitePeaks", "SatellitePeaks");
    IPeaksWorkspace_sptr SatellitePeaks2 = alg4.getProperty("SatellitePeaks");
    TS_ASSERT(SatellitePeaks2);

    TS_ASSERT_EQUALS(SatellitePeaks2->getNumberPeaks(), 939);

    AnalysisDataService::Instance().remove("Modulated");
  }

  void test_exec_multiple_goniometers() {
    /* create a TOPAZ instrument */
    FrameworkManager::Instance().exec("LoadEmptyInstrument", 4, "InstrumentName", "TOPAZ", "OutputWorkspace", "inst");

    /* Add it to a Peaks workspace */
    FrameworkManager::Instance().exec("CreatePeaksWorkspace", 6, "InstrumentWorkspace", "inst", "NumberOfPeaks", "0",
                                      "OutputWorkspace", "peaks");

    /* retrieve a handle to the peaks workspace created in the FrameworkManager
     */
    PeaksWorkspace_sptr peaks_workspace;
    TS_ASSERT_THROWS_NOTHING(
        peaks_workspace = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("peaks")));

    /* set uniform background */
    FrameworkManager::Instance().exec("SetUB", 8, "Workspace", "peaks", "a", "2", "b", "2", "c", "2");

    /* create 2 peaks */
    FrameworkManager::Instance().exec("AddPeakHKL", 4, "Workspace", "peaks", "HKL", "1,1,0");

    peaks_workspace->getPeak(0).setRunNumber(0);

    /* move the goniometer to point to the 2nd peak */
    FrameworkManager::Instance().exec("SetGoniometer", 4, "Workspace", "peaks", "Axis0", "180,0,1,0,1");

    FrameworkManager::Instance().exec("AddPeakHKL", 4, "Workspace", "peaks", "HKL", "-1,-1,0");

    peaks_workspace->getPeak(1).setRunNumber(1);

    /* algorithm to test */
    PredictSatellitePeaks alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("Peaks", "peaks");
    alg.setProperty("SatellitePeaks", std::string("SatellitePeaks"));
    alg.setProperty("ModVector1", "0.2,0,0");
    alg.setProperty("MaxOrder", "1");
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    alg.setPropertyValue("SatellitePeaks", "SatellitePeaks");

    IPeaksWorkspace_sptr SatelliteIPeaks2 = alg.getProperty("SatellitePeaks");
    PeaksWorkspace_sptr SatellitePeaks2 = std::dynamic_pointer_cast<PeaksWorkspace>(SatelliteIPeaks2);

    TS_ASSERT(SatellitePeaks2);
    TS_ASSERT_EQUALS(SatellitePeaks2->getNumberPeaks(), 4);

    /* pull out the satellite peaks */
    std::vector<Peak> const &satellite_peaks = SatellitePeaks2->getPeaks();

    /* check goniometer matrices */

    TS_ASSERT_EQUALS(satellite_peaks[0].getGoniometerMatrix(), Mantid::Kernel::Matrix<double>(3, 3, true));

    TS_ASSERT_EQUALS(satellite_peaks[1].getGoniometerMatrix(), Mantid::Kernel::Matrix<double>(3, 3, true));

    TS_ASSERT_EQUALS(satellite_peaks[2].getGoniometerMatrix(),
                     Mantid::Kernel::Matrix<double>({-1, 0, 0, 0, 1, 0, 0, 0, -1}));

    TS_ASSERT_EQUALS(satellite_peaks[3].getGoniometerMatrix(),
                     Mantid::Kernel::Matrix<double>({-1, 0, 0, 0, 1, 0, 0, 0, -1}));

    /* check peak HKL coordinates */
    TS_ASSERT_EQUALS(satellite_peaks[0].getHKL(), Mantid::Kernel::V3D(1.2, 1, 0));
    TS_ASSERT_EQUALS(satellite_peaks[1].getHKL(), Mantid::Kernel::V3D(0.8, 1, 0));
    TS_ASSERT_EQUALS(satellite_peaks[2].getHKL(), Mantid::Kernel::V3D(-1.2, -1, 0));
    TS_ASSERT_EQUALS(satellite_peaks[3].getHKL(), Mantid::Kernel::V3D(-0.8, -1, 0));

    /* check run numbers */
    TS_ASSERT_EQUALS(satellite_peaks[0].getRunNumber(), 0);
    TS_ASSERT_EQUALS(satellite_peaks[1].getRunNumber(), 0);
    TS_ASSERT_EQUALS(satellite_peaks[2].getRunNumber(), 1);
    TS_ASSERT_EQUALS(satellite_peaks[3].getRunNumber(), 1);

    AnalysisDataService::Instance().remove("peaks");
  }

  void test_exec_multiple_goniometers_lean() {

    /* Add it to a Peaks workspace */
    TS_ASSERT_THROWS_NOTHING(FrameworkManager::Instance().exec(
        "CreatePeaksWorkspace", 6, "NumberOfPeaks", "0", "OutputWorkspace", "peaks", "OutputType", "LeanElasticPeak"));

    /* retrieve a handle to the peaks workspace created in the FrameworkManager
     */
    LeanElasticPeaksWorkspace_sptr peaks_workspace;
    TS_ASSERT_THROWS_NOTHING(peaks_workspace = std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(
                                 AnalysisDataService::Instance().retrieve("peaks")));

    /* set uniform background */
    FrameworkManager::Instance().exec("SetUB", 8, "Workspace", "peaks", "a", "2", "b", "2", "c", "2");

    /* create 2 peaks */
    FrameworkManager::Instance().exec("AddPeakHKL", 4, "Workspace", "peaks", "HKL", "1,1,0");

    peaks_workspace->getPeak(0).setRunNumber(0);

    /* move the goniometer to point to the 2nd peak */
    FrameworkManager::Instance().exec("SetGoniometer", 4, "Workspace", "peaks", "Axis0", "180,0,1,0,1");

    FrameworkManager::Instance().exec("AddPeakHKL", 4, "Workspace", "peaks", "HKL", "-1,-1,0");

    peaks_workspace->getPeak(1).setRunNumber(1);

    /* algorithm to test */
    PredictSatellitePeaks alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("Peaks", "peaks");
    alg.setProperty("SatellitePeaks", std::string("SatellitePeaks"));
    alg.setProperty("ModVector1", "0.2,0,0");
    alg.setProperty("MaxOrder", "1");
    alg.setProperty("IncludeIntegerHKL", false);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    alg.setPropertyValue("SatellitePeaks", "SatellitePeaks");

    IPeaksWorkspace_sptr SatelliteIPeaks2 = alg.getProperty("SatellitePeaks");
    LeanElasticPeaksWorkspace_sptr SatellitePeaks2 =
        std::dynamic_pointer_cast<LeanElasticPeaksWorkspace>(SatelliteIPeaks2);

    TS_ASSERT(SatellitePeaks2);
    TS_ASSERT_EQUALS(SatellitePeaks2->getNumberPeaks(), 4);

    /* pull out the satellite peaks */
    std::vector<LeanElasticPeak> const &satellite_peaks = SatellitePeaks2->getPeaks();

    /* check goniometer matrices */

    TS_ASSERT_EQUALS(satellite_peaks[0].getGoniometerMatrix(), Mantid::Kernel::Matrix<double>(3, 3, true));

    TS_ASSERT_EQUALS(satellite_peaks[1].getGoniometerMatrix(), Mantid::Kernel::Matrix<double>(3, 3, true));

    TS_ASSERT_EQUALS(satellite_peaks[2].getGoniometerMatrix(),
                     Mantid::Kernel::Matrix<double>({-1, 0, 0, 0, 1, 0, 0, 0, -1}));

    TS_ASSERT_EQUALS(satellite_peaks[3].getGoniometerMatrix(),
                     Mantid::Kernel::Matrix<double>({-1, 0, 0, 0, 1, 0, 0, 0, -1}));

    TS_ASSERT_EQUALS(satellite_peaks[1].getHKL(), Mantid::Kernel::V3D(1.2, 1, 0));
    TS_ASSERT_EQUALS(satellite_peaks[0].getHKL(), Mantid::Kernel::V3D(0.8, 1, 0));

    TS_ASSERT_EQUALS(satellite_peaks[2].getHKL(), Mantid::Kernel::V3D(-1.2, -1, 0));
    TS_ASSERT_EQUALS(satellite_peaks[3].getHKL(), Mantid::Kernel::V3D(-0.8, -1, 0));

    /* check run numbers */
    TS_ASSERT_EQUALS(satellite_peaks[0].getRunNumber(), 0);
    TS_ASSERT_EQUALS(satellite_peaks[1].getRunNumber(), 0);
    TS_ASSERT_EQUALS(satellite_peaks[2].getRunNumber(), 1);
    TS_ASSERT_EQUALS(satellite_peaks[3].getRunNumber(), 1);

    AnalysisDataService::Instance().remove("peaks");
  }
};
