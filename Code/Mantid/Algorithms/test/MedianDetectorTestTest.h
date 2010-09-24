#ifndef WBVMEDIANTESTTEST_H_
#define WBVMEDIANTESTTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/MedianDetectorTest.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadInstrument.h"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cmath>
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

class MedianDetectorTestTest : public CxxTest::TestSuite
{
public:

  void testWorkspaceAndArray()
  {
    MedianDetectorTest alg;
    TS_ASSERT_EQUALS( alg.name(), "MedianDetectorTest" )
    TS_ASSERT_EQUALS( alg.version(), 1 )
    //the spectra were setup in the constructor and passed to our algorithm through this function
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( runInit(alg) ) )

    alg.setProperty("SignificanceTest", 1.0);
    //these are realistic values that I just made up
    alg.setProperty( "LowThreshold", 0.5 );
    alg.setProperty( "HighThreshold", 1.3333 );
    //we are using the defaults on StartSpectrum, EndSpectrum, RangeLower and RangeUpper which is to use the whole spectrum

    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    std::vector<int> ZeroArray;
    TS_ASSERT_THROWS_NOTHING( ZeroArray = alg.getProperty("BadSpectraNums") )
    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("MedianDetectorTestOutput"));

    MatrixWorkspace_const_sptr input;
    input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(m_IWSName));
    TS_ASSERT(input)
    TS_ASSERT(
      input->getInstrument()->getDetector(THEMASKED).get()->isMasked()
      )

    MatrixWorkspace_sptr outputMat = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    TS_ASSERT ( outputMat ) ;
    TS_ASSERT_EQUALS( outputMat->YUnit(), "" )

    //There are three outputs, a workspace (tested in the next code block), an array (later, this test) and a file (next test)
    //were all the spectra output?
		const int numberOfSpectra = outputMat->getNumberHistograms();
    TS_ASSERT_EQUALS(numberOfSpectra, (int)Nhist);
    // the numbers below are threshold values that were found by trail and error running these tests
    const int firstGoodSpec = 36;
    const int lastGoodSpec = 95;
    for (int lHist = 1; lHist < firstGoodSpec; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), BAD_VAL )
    }
    for (int lHist=firstGoodSpec; lHist < THEMASKED-1; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), GOOD_VAL )
    }
    TS_ASSERT_EQUALS(
        outputMat->readY(THEMASKED-1).front(), BAD_VAL )
    for (int lHist=THEMASKED; lHist <= lastGoodSpec; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), GOOD_VAL )
    }
    for (int lHist = lastGoodSpec+1; lHist < SAVEDBYERRORBAR; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), BAD_VAL )
    }
    for (int lHist = SAVEDBYERRORBAR; lHist < Nhist; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), GOOD_VAL )
    }
    //now check the array
    TS_ASSERT_DIFFERS(ZeroArray.size(), 0 )

    std::vector<int>::const_iterator it = ZeroArray.begin();
    for (int lHist = 0 ; lHist < firstGoodSpec; lHist++ )
    {
      TS_ASSERT_EQUALS( *it, lHist+1 )
      ++it;
    }
    for (int lHist = lastGoodSpec+1 ; lHist < SAVEDBYERRORBAR ; lHist++ )
    {
      TS_ASSERT_EQUALS( *it, lHist+1 )
      ++it;
    }
    //check that extra entries haven't been written to the array
    TS_ASSERT_EQUALS( it, ZeroArray.end() )
  }

  void testFile()
  {
    MedianDetectorTest alg;
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( runInit(alg) ) )

    // values a little extreme, I just made them up
    alg.setProperty( "LowThreshold", 0.44444 );
    alg.setProperty( "HighThreshold", 5.0 );
    // this should turn off examining the errorbars.  This makes things simplier and significance testing was tested in the last test
    alg.setProperty("SignificanceTest", 0.0);

    const int fSpec = 0,  lSpec = Nhist/2;
    alg.setProperty( "StartWorkspaceIndex", fSpec );
    alg.setProperty( "EndWorkspaceIndex", lSpec );
    //a couple of random numbers in the range
    const double lRange = 4000, uRange = 10000;
    alg.setProperty( "RangeLower", lRange );
    alg.setProperty( "RangeUpper", uRange );
    alg.setPropertyValue( "OutputFile", m_OFileName );

    // this is a summary of the results, I got these numbers by running the test in the debugger
    const int firstGoodSpec = 16;
    // there are no good spectra here
    const int lastGoodSpec = Nhist;

    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    //test file output
    std::ifstream testFile(m_OFileName.c_str(), std::ios::in);
    TS_ASSERT ( testFile )
    
    std::string fileLine;
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "---"+alg.name()+"---" )
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, std::string("----Low Integral : ")+boost::lexical_cast<std::string>(firstGoodSpec)+std::string("----") )
      
    std::string correct;
    for (int iHist = 0 ; iHist < firstGoodSpec; iHist++ )
    {
      correct += boost::lexical_cast<std::string>(m_2DWS->getAxis(1)->spectraNo(iHist));
      if ( (iHist + 1) % 10 == 0 || iHist == firstGoodSpec-1 ) 
      {
        std::getline( testFile, fileLine );
        TS_ASSERT_EQUALS ( fileLine, correct )
        correct.clear();
      }
      else
      {
        correct += " ";
      }
    }

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, std::string("----High Integral : ")+boost::lexical_cast<std::string>(Nhist-lastGoodSpec)+std::string("----") )
      
    for (int iHist = lastGoodSpec ; iHist < Nhist; iHist++ )
    {
      correct += boost::lexical_cast<std::string>(m_2DWS->getAxis(1)->spectraNo(iHist));
      if ( (iHist + 1) % 10 == 0 || iHist == firstGoodSpec-1 )
      {
        std::getline( testFile, fileLine );
        TS_ASSERT_EQUALS ( fileLine, correct )
        correct.clear();
      }
      else
      {
        correct += " ";
      }
    }
    if ( ! correct.empty() )
    {
      std::getline( testFile, fileLine );
      TS_ASSERT_EQUALS ( fileLine, correct )
    }
    testFile.close();
    Poco::File(m_OFileName.c_str()).remove();
  }
    
  MedianDetectorTestTest() : m_IWSName("MedianDetectorTestInput"),
    m_OFileName("")
  {
    m_OFileName = Poco::Path().absolute().resolve("MedianDetectorTestTestFile.txt").toString();
    using namespace Mantid;
    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",Nhist,11,10);
    m_2DWS = boost::dynamic_pointer_cast<Workspace2D>(space);
    const short specLength = 22;
    boost::shared_ptr<MantidVec> x(new MantidVec(specLength));
    for (int i = 0; i < specLength; ++i)
    {
      (*x)[i]=i*1000;
    }
    // the data will be 21 random numbers
    double yArray[specLength-1] =
      {0.2,4,50,0.001,0,0,0,1,0,15,4,0,0.001,2e-10,0,8,0,1e-4,1,7,11};
    m_YSum = 0; for (int i = 0; i < specLength-1; i++) m_YSum += yArray[i];

    // most error values will be small so that they wont affect the tests
    boost::shared_ptr<MantidVec> smallErrors( 
      new MantidVec( specLength-1, 0.01*m_YSum/specLength ) );
    // if the SignificanceTest property is set to one, knowing what happens in the loop below, these errors will just make or break the tests
    boost::shared_ptr<MantidVec> almostBigEnough( new MantidVec( specLength-1, 0) );
    (*almostBigEnough)[0] = 0.9*m_YSum*(0.5*Nhist-1);
    boost::shared_ptr<MantidVec> bigEnough( new MantidVec( specLength-1, 0 ) );
    (*bigEnough)[0] = 1.2*m_YSum*(0.5*Nhist);

    int forSpecDetMap[Nhist];
    for (int j = 0; j < Nhist; ++j)
    {
      m_2DWS->setX(j, x);
      boost::shared_ptr<MantidVec> spectrum( new MantidVec );
      //the spectravalues will be multiples of the random numbers above
      for ( int l = 0; l < specLength-1; ++l )
      {
        spectrum->push_back( j*yArray[l] );
      }
      boost::shared_ptr<MantidVec> errors = smallErrors;
      if ( j == Nhist-2 ) errors = almostBigEnough;
      if ( j == SAVEDBYERRORBAR ) errors = bigEnough;

      m_2DWS->setData( j, spectrum, errors );
      // Just set the spectrum number to match the index
      m_2DWS->getAxis(1)->spectraNo(j) = j+1;
      forSpecDetMap[j] = j+1;
    }

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(m_IWSName, space);
    
    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "../../../../Test/Instrument/INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", m_IWSName);
    loader.execute(); 

    m_2DWS->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, Nhist);

    m_2DWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    // mask the detector
    Geometry::ParameterMap* m_Pmap = &(m_2DWS->instrumentParameters());    
    boost::shared_ptr<Instrument> instru = m_2DWS->getBaseInstrument();
    Geometry::Detector* toMask =
      dynamic_cast<Geometry::Detector*>( instru->getDetector(THEMASKED).get() );
    TS_ASSERT(toMask)
    m_Pmap->addBool(toMask, "masked", true);
  }

  bool runInit(MedianDetectorTest &alg)
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    bool good = alg.isInitialized();

    // Set the properties
    alg.setPropertyValue("InputWorkspace", m_IWSName);
    alg.setPropertyValue("OutputWorkspace","MedianDetectorTestOutput");
    return good;
  }

private:
  std::string m_IWSName, m_OFileName;
  Workspace2D_sptr m_2DWS;
  double m_YSum;
  enum spectraIndexConsts{ THEMASKED = 40, SAVEDBYERRORBAR = 143, Nhist = 144 };
  //these values must match the values in MedianDetectorTest.h
  enum FLAGS{ BAD_VAL = 100, GOOD_VAL = 0 };

};

#endif /*WBVMEDIANTESTTEST_H_*/
