#ifndef FIND_SX_UB_USING_LATTICE_PARAMETERS_TEST_H
#define FIND_SX_UB_USING_LATTICE_PARAMETERS_TEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/ITableWorkspace.h";
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidCrystal/FindSXUBUsingLatticeParameters.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidAPI/IPeak.h"
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;


//=====================================================================================
// Functional tests
//=====================================================================================
class FindSXUBUsingLatticeParameterTest : public CxxTest::TestSuite
{

private:

  //Master copy of existing peaks workspace
  Mantid::DataObjects::PeaksWorkspace_sptr m_masterPeaks;

public:

  FindSXUBUsingLatticeParameterTest()
  {
    //Load an existing peaks workspace. This workspace already has HKL values.
    std::string WSName("peaks");
    LoadIsawPeaks loader;
    TS_ASSERT_THROWS_NOTHING( loader.initialize() );
    TS_ASSERT( loader.isInitialized() );
    loader.setPropertyValue("Filename", "TOPAZ_3007.peaks");
    loader.setPropertyValue("OutputWorkspace", WSName);
    //Execute and fetch the workspace
    loader.execute();
    m_masterPeaks  = boost::dynamic_pointer_cast<PeaksWorkspace>(
        AnalysisDataService::Instance().retrieve(WSName) );
  }

  ~FindSXUBUsingLatticeParameterTest()
  {
    AnalysisDataService::Instance().remove("peaks");
  }

  void doTest(int nPixels, std::string peakIndexes, double a, double b, double c, double alpha, double beta, double gamma, double dTolerance=0.01)
  {
    
    //Take a copy of the original peaks workspace.
    PeaksWorkspace_sptr local = m_masterPeaks->clone();
    AnalysisDataService::Instance().addOrReplace("PeaksWS", local);

    FindSXUBUsingLatticeParameters alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", "PeaksWS") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("a", a) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("b", b) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("c", c) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("alpha", alpha) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("beta", beta) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("gamma", gamma) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeakIndices",peakIndexes) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("dTolerance", dTolerance) );
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    //This particular input workspace already has HKL values, so we check that those calculated are the same as the original.
    for(int i = 0; i < nPixels; i++)
    {
      IPeak& peakMaster = m_masterPeaks->getPeak(i);
      IPeak& peakModified = local->getPeak(i);
      TSM_ASSERT_EQUALS("Wrong H value", peakMaster.getH(), peakModified.getH() );
      TSM_ASSERT_EQUALS("Wrong K value", peakMaster.getK(), peakModified.getK() );
      TSM_ASSERT_EQUALS("Wrong L value", peakMaster.getL(), peakModified.getL() );
    }

    //Clean-up
    AnalysisDataService::Instance().remove("PeaksWS");
  }

  void test_lessThanTwoPeaksThrows()
  {
    TS_ASSERT_THROWS(doTest(1, "1", 14.131, 19.247, 8.606, 90.0, 105.071, 90.0), std::runtime_error);
  }

  void test_colinearPeaksThrows()
  {
    PeaksWorkspace_sptr temp = m_masterPeaks->clone();

    for(int i = 0; i < m_masterPeaks->getNumberPeaks(); i++)
    {
      IPeak& peak = m_masterPeaks->getPeak(i);
      Mantid::Kernel::V3D v(1, 0, 0);
      peak.setQSampleFrame(v); // Overwrite all Q samples to be co-linear.
    }

    TS_ASSERT_THROWS(doTest(6, "1, 2, 3, 4, 5, 6", 14.131, 19.247, 8.606, 90.0, 105.071, 90.0), std::runtime_error);

    //Restore master. peaks workspace.
    m_masterPeaks = temp;
  }

  void test_exec()
  {
    doTest(6, "1, 2, 3, 4, 5, 6", 14.131, 19.247, 8.606, 90.0, 105.071, 90.0);
  }

  void test_perturbateA()
  {
    //a increased to 15
    doTest(6, "1, 2, 3, 4, 5, 6", 15.00, 19.247, 8.606, 90.0, 105.071, 90.0);
  }

  void test_perturbateB()
  {
    //b increased to 20
    doTest(6, "1, 2, 3, 4, 5, 6", 14.131, 20.00, 8.606, 90.0, 105.071, 90.0);
  }

  void test_perturbateC()
  {
    //c increased to 9
    doTest(6, "1, 2, 3, 4, 5, 6", 14.131, 19.247, 9.00, 90.0, 105.071, 90.0);
  }

  void test_perturbateAlpha()
  {
    //Alpha decreased to 89
    doTest(6, "1, 2, 3, 4, 5, 6", 14.131, 19.247, 8.606, 89.0, 105.071, 90.0);
  }

  void test_perturbateBeta()
  {
    //Beta increased to 108
    doTest(6, "1, 2, 3, 4, 5, 6", 14.131, 19.247, 8.606, 90.0, 108.00, 90.0);
  }

  void test_perturbateGamma()
  {
    //Gamma decreased to 88
    doTest(6, "1, 2, 3, 4, 5, 6", 14.131, 19.247, 8.606, 90.0, 105.071, 88.0);
  }



};

#endif
