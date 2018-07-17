/*
 * PredictSatellitePeaksTest.h
 *
 *  Created on: Dec 28, 2012
 *      Author: ruth
 */

#ifndef PREDICTFRACTIONALPEAKSTEST_H_
#define PREDICTFRACTIONALPEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidCrystal/PredictSatellitePeaks.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

class PredictSatellitePeaksTest : public CxxTest::TestSuite {

public:
  void test_Init() {
    PredictSatellitePeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec() {
    LoadNexusProcessed loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());

    std::string WSName("peaks");
    loader.setPropertyValue("Filename", "TOPAZ_3007.peaks.nxs");
    loader.setPropertyValue("OutputWorkspace", WSName);
    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());

    LoadIsawUB UBloader;
    TS_ASSERT_THROWS_NOTHING(UBloader.initialize());
    TS_ASSERT(UBloader.isInitialized());

    UBloader.setPropertyValue("InputWorkspace", WSName);

    UBloader.setProperty("FileName", "TOPAZ_3007.mat");
    TS_ASSERT(UBloader.execute());
    TS_ASSERT(UBloader.isExecuted());

    IndexPeaks indexer;
    TS_ASSERT_THROWS_NOTHING(indexer.initialize());
    TS_ASSERT(indexer.isInitialized());

    indexer.setPropertyValue("PeaksWorkspace", WSName);

    TS_ASSERT(indexer.execute());
    TS_ASSERT(indexer.isExecuted());

    PredictSatellitePeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("Peaks", WSName);

    alg.setProperty("SatellitePeaks", std::string("SatellitePeaks"));
    alg.setProperty("ModVector1", "0.5,0,.2");
    alg.setProperty("MaxOrder", "1");
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());
    alg.setPropertyValue("SatellitePeaks", "SatellitePeaks");
    PeaksWorkspace_sptr SatellitePeaks = alg.getProperty("SatellitePeaks");

    TS_ASSERT_EQUALS(SatellitePeaks->getNumberPeaks(), 122);

    auto &peak0 = SatellitePeaks->getPeak(0);
    TS_ASSERT_DELTA(peak0.getH(), -5.5, .0001);
    TS_ASSERT_DELTA(peak0.getK(), 7.0, .0001);
    TS_ASSERT_DELTA(peak0.getL(), -4.2, .0001);

    auto &peak3 = SatellitePeaks->getPeak(3);
    TS_ASSERT_DELTA(peak3.getH(), -5.5, .0001);
    TS_ASSERT_DELTA(peak3.getK(), 3.0, .0001);
    TS_ASSERT_DELTA(peak3.getL(), -3.2, .0001);

    auto &peak6 = SatellitePeaks->getPeak(6);
    TS_ASSERT_DELTA(peak6.getH(), -6.5, .0001);
    TS_ASSERT_DELTA(peak6.getK(), 4.0, .0001);
    TS_ASSERT_DELTA(peak6.getL(), -4.2, .0001);
  }
};

#endif /* PredictFRACTIONALPEAKSTEST_H_ */
