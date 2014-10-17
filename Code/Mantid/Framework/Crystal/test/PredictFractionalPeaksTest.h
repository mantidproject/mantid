/*
 * PredictFractionalPeaksTest.h
 *
 *  Created on: Dec 28, 2012
 *      Author: ruth
 */

#ifndef PREDICTFRACTIONALPEAKSTEST_H_
#define PREDICTFRACTIONALPEAKSTEST_H_


#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"


#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidCrystal/PredictFractionalPeaks.h"
#include "MantidAPI/FrameworkManager.h"


using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class PredictFractionalPeaksTest: public CxxTest::TestSuite
{

public:
 void test_Init()
    {
      PredictFractionalPeaks alg;
      TS_ASSERT_THROWS_NOTHING( alg.initialize() );
      TS_ASSERT( alg.isInitialized() );
    }

  void test_exec()
  {
    LoadIsawPeaks loader;
    TS_ASSERT_THROWS_NOTHING( loader.initialize() );
    TS_ASSERT( loader.isInitialized() );

    std::string WSName("peaks");
    loader.setPropertyValue("Filename", "TOPAZ_3007.peaks");
    loader.setPropertyValue("OutputWorkspace", WSName);
    TS_ASSERT( loader.execute());
    TS_ASSERT(loader.isExecuted());

    LoadIsawUB UBloader;
    TS_ASSERT_THROWS_NOTHING( UBloader.initialize());
    TS_ASSERT(UBloader.isInitialized());

    UBloader.setPropertyValue("InputWorkspace",WSName);

    UBloader.setProperty("FileName","TOPAZ_3007.mat");
    TS_ASSERT( UBloader.execute());
    TS_ASSERT(UBloader.isExecuted());


    IndexPeaks indexer;
    TS_ASSERT_THROWS_NOTHING(indexer.initialize());
    TS_ASSERT( indexer.isInitialized());

    indexer.setPropertyValue("PeaksWorkspace",WSName);

    TS_ASSERT( indexer.execute());
    TS_ASSERT(indexer.isExecuted());

    PredictFractionalPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty( "Peaks",WSName);


    alg.setProperty("FracPeaks",std::string("FracPeaks"));
    alg.setProperty("HOffset","-.5,0,.5");
    alg.setProperty("KOffset","0.0");
    alg.setProperty("LOffset",".2");
    TS_ASSERT( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
    alg.setPropertyValue("FracPeaks","FracPeaks");
    IPeaksWorkspace_sptr FracPeaks = alg.getProperty("FracPeaks");

    TS_ASSERT_EQUALS(FracPeaks->getNumberPeaks(),117);


    Peak& peakx=dynamic_cast<Peak&>(FracPeaks->getPeak(0));
    Peak peak=peakx;
    TS_ASSERT_DELTA(peak.getH(),-5.5,.0001);
    TS_ASSERT_DELTA(peak.getK(),7.0,.0001);
    TS_ASSERT_DELTA(peak.getL(),-3.8,.0001);


    peakx=dynamic_cast<Peak&>(FracPeaks->getPeak(3));
    peak=peakx;
    TS_ASSERT_DELTA(peak.getH(),-5.5,.0001);
    TS_ASSERT_DELTA(peak.getK(),3.0,.0001);
    TS_ASSERT_DELTA(peak.getL(),-2.8,.0001);

    peakx=dynamic_cast<Peak&>(FracPeaks->getPeak(6));
    peak=peakx;
    TS_ASSERT_DELTA(peak.getH(),-6.5,.0001);
    TS_ASSERT_DELTA(peak.getK(),4.0,.0001);
    TS_ASSERT_DELTA(peak.getL(),-3.8,.0001);


  }
};

#endif /* PredictFRACTIONALPEAKSTEST_H_ */
