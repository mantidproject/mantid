#ifndef DETECTOREFFICIENCYVARIATION_H_
#define DETECTOREFFICIENCYVARIATION_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/DetectorEfficiencyVariation.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadInstrument.h"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <Poco/Path.h>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ios>
#include <string>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class DetectorEfficiencyVariationTest : public CxxTest::TestSuite
{
public:
  bool runInit(DetectorEfficiencyVariation &alg)//this is run by both tests so I thought I'd take it out and split things up
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    bool good = alg.isInitialized();
    // Set the properties
    alg.setPropertyValue("WhiteBeamBase", m_WB1Name);
    alg.setPropertyValue("WhiteBeamCompare", m_WB2Name);
    alg.setPropertyValue("OutputWorkspace", "DetEfficVariTestWSO");
    return good;
  }

  void testWorkspace()
  {
    DetectorEfficiencyVariation alg;
    //the spectra were setup in the constructor and passed to our algorithm through this function
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( runInit(alg) ) )

    //these are realistic values that I just made up
    double variation = 1.1;
    alg.setProperty( "Variation", variation );

    //we are using the defaults on StartSpectrum, EndSpectrum, 
    //RangeLower and RangeUpper which is to use the whole spectrum

    alg.setRethrows(true);

    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("DetEfficVariTestWSO"));
    MatrixWorkspace_sptr outputMat = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    TS_ASSERT ( outputMat ) ;
    TS_ASSERT_EQUALS( outputMat->YUnit(), "" );
    
    const int numFailed = alg.getProperty("NumberOfFailures");
    TS_ASSERT_EQUALS(numFailed, 65);

    int firstGoodSpec = (Nhist/2)-int((variation-1)/m_ramp)+1;
    int lastGoodSpec = (Nhist/2)+int((variation-1)/m_ramp)-1;
    for (int lHist = 0; lHist < firstGoodSpec; lHist++)
    {
      TS_ASSERT_EQUALS(static_cast<double>(outputMat->readY(lHist).front()),
        static_cast<double>(BadVal) )
    }
    for (int lHist = firstGoodSpec; lHist <= lastGoodSpec; lHist++)
    {
      TS_ASSERT_EQUALS(static_cast<double>(outputMat->readY(lHist).front()),
        static_cast<double>(GoodVal) )
    }
    for (int lHist = lastGoodSpec+1; lHist < Nhist; lHist++)
    {
      TS_ASSERT_EQUALS( static_cast<double>(outputMat->readY(lHist).front()),
        static_cast<double>(BadVal) )
    }
  }
       
  DetectorEfficiencyVariationTest() :
    m_WB1Name("DetEfficVariTestWSI1"), m_WB2Name("DetEfficVariTestWSI2"), m_ramp(0.01)
  {
    using namespace Mantid;
    // Set up a small workspace for testing
    Workspace_sptr spaceA = WorkspaceFactory::Instance().create("Workspace2D",Nhist,NXs,NXs-1);
    Workspace_sptr spaceB = WorkspaceFactory::Instance().create("Workspace2D",Nhist,NXs,NXs-1);
    Workspace2D_sptr inputA = boost::dynamic_pointer_cast<Workspace2D>(spaceA);
    Workspace2D_sptr inputB = boost::dynamic_pointer_cast<Workspace2D>(spaceB);
    boost::shared_ptr<MantidVec> x(new MantidVec(NXs));
    for (int i = 0; i < NXs; ++i)
    {
      (*x)[i]=i*1000;
    }
    // random numbers that will be copied into the workspace spectra
    const short ySize = NXs-1;
    double yArray[ySize] =
      {0.2,4,50,14,0.001,0,0,0,1,0,1e-3,15,4,0,9,0.001,2e-10,1,0,8,0,7,1e-4,1,
      7,11,101,6,53,0.345324,3444,13958,0.8};//NXs = 34 so we need that many numbers

    //the error values aren't used and aren't tested so we'll use some basic data
    boost::shared_ptr<MantidVec> errors( new MantidVec( ySize, 1) );
    boost::shared_ptr<MantidVec> forInputA, forInputB;

    int forSpecDetMap[Nhist];
    for (int j = 0; j < Nhist; ++j)
    {
      inputA->setX(j, x);
      // both workspaces must have the same x bins
      inputB->setX(j, x);
      forInputA.reset( new MantidVec );
      forInputB.reset( new MantidVec );
      // the spectravalues will be multiples of the random numbers above
      for ( int l = 0; l < ySize; ++l )
      {
        forInputA->push_back(yArray[l]);
        //there is going to be a small difference between the workspaces that will vary with histogram number
        forInputB->push_back( forInputA->back()*( 1+m_ramp*(j-(Nhist/2)) ) );
      }
      // insert a particularly large value to pick up later
      m_LargeValue = 3.1;
      if ( j == Nhist-1 )
        for ( int l = 0; l < ySize; ++l )
          (*forInputB)[l] = (*forInputA)[l]*m_LargeValue;

      inputA->setData( j, forInputA, errors );
      inputB->setData( j, forInputB, errors );
      // Just set the spectrum number to match the index, spectra numbers and detector maps must be indentical for both 
      inputA->getAxis(1)->spectraNo(j) = j+1;
      inputB->getAxis(1)->spectraNo(j) = j+1;
      forSpecDetMap[j] = j+1;
    }

    // Register the input workspaces to the ADS where they can be accessed by the algorithm
    AnalysisDataService::Instance().add(m_WB1Name, inputA);
    AnalysisDataService::Instance().add(m_WB2Name, inputB);
    
    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "../../../Instrument/INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", m_WB1Name);
    loader.execute(); 
    //both workspaces should use the same instrument information
    loader.setPropertyValue("Workspace", m_WB2Name);
    loader.execute(); 

    inputA->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, Nhist);
    inputB->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, Nhist);
    inputA->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    inputB->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  }

private:
  std::string m_WB1Name, m_WB2Name;
  double m_ramp, m_LargeValue;
  enum { Nhist = 84, NXs = 34, 
    //these values must match the values in DetectorEfficiencyVariation.h
    BadVal  = 0, GoodVal = 1 };
};


#endif /*DETECTOREFFICIENCYVARIATION_H_*/
