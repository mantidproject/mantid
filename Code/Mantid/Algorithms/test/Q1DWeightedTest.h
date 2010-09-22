#ifndef Q1DWEIGHTEDTEST_H_
#define Q1DWEIGHTEDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Q1DWeighted.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidAlgorithms/SolidAngleCorrection.h"
#include <iostream>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class Q1DWeightedTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( radial_average.name(), "Q1DWeighted" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( radial_average.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( radial_average.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( radial_average.initialize() )
    TS_ASSERT( radial_average.isInitialized() )
  }

  void testExec()
  {
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/AutoTestData/BioSANS_exp61_scan0004_0001.xml");
    const std::string inputWS("wav");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();

    // Move detector to its correct position
    Mantid::DataHandling::MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace",inputWS);
    mover.setPropertyValue("ComponentName","detector1");

    // According to the instrument geometry, the center of the detector is located at N_pixel / 2 + 0.5
    // X = (16-192.0/2.0+0.5)*5.15/1000.0 = -0.409425
    // Y = (95-192.0/2.0+0.5)*5.15/1000.0 = -0.002575
    mover.setPropertyValue("X","0.409425");
    mover.setPropertyValue("Y","0.002575");
    mover.execute();

    // Perform solid angle correction
    Mantid::Algorithms::SolidAngleCorrection solidcorr;
    solidcorr.initialize();
    solidcorr.setPropertyValue("InputWorkspace",inputWS);
    solidcorr.setPropertyValue("OutputWorkspace",inputWS);
    solidcorr.execute();

    if (!radial_average.isInitialized()) radial_average.initialize();

    TS_ASSERT_THROWS_NOTHING( radial_average.setPropertyValue("InputWorkspace",inputWS) )
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING( radial_average.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( radial_average.setPropertyValue("OutputBinning","0.01,0.001,0.11") )
    TS_ASSERT_THROWS_NOTHING( radial_average.setPropertyValue("NPixelDivision", "3") )
    TS_ASSERT_THROWS_NOTHING( radial_average.setPropertyValue("ErrorWeighting", "1") )

    TS_ASSERT_THROWS_NOTHING( radial_average.execute() )

    TS_ASSERT( radial_average.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_EQUALS( result->getNumberHistograms(), 1 )

    // Timer is 3600.0 for this test data file
    double tolerance(1e-03);

    // The points we are checking were computed using the HFIR IGOR package
    // For NPixelDivision = 1
    //   Y[1] = 0.0398848*3600; Y[2] = 0.0371762*3600; Y[30] = 0.030971*3600; Y[80] = 0.0275545*3600; Y[90] = 0.0270528*3600
    TS_ASSERT_EQUALS( result->dataX(0)[0], 0.01);
    double iq = 0.0308929*3600;
    TS_ASSERT_DELTA( result->dataY(0)[30], iq, tolerance);
    iq = 0.0397903*3600;
    TS_ASSERT_DELTA( result->dataY(0)[1], iq, tolerance);
    iq = 0.0373098*3600;
    TS_ASSERT_DELTA( result->dataY(0)[2], iq, tolerance);
    iq = 0.0276372*3600;
    TS_ASSERT_DELTA( result->dataY(0)[80], iq, tolerance);
    iq = 0.0270194*3600;
    TS_ASSERT_DELTA( result->dataY(0)[90], iq, tolerance);

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);

  }

private:
  Mantid::Algorithms::Q1DWeighted radial_average;
  std::string inputWS;
};

#endif /*Q1DWEIGHTEDTEST_H_*/
