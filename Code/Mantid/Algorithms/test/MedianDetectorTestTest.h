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
#include <math.h>
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

    std::vector<int> OArray;
    TS_ASSERT_THROWS_NOTHING( OArray = alg.getProperty( "BadDetectorIDs" ) )
    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("MedianDetectorTestOutput"));
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(m_IWSName));
    MatrixWorkspace_sptr outputMat = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    TS_ASSERT ( outputMat ) ;
    TS_ASSERT_EQUALS( outputMat->YUnit(), "" )

    //There are three outputs, a workspace (tested in the next code block), an array (later, this test) and a file (next test)
    //were all the spectra output?
		const int numberOfSpectra = outputMat->getNumberHistograms();
    TS_ASSERT_EQUALS(numberOfSpectra, (int)Nhist);
    // the numbers below are threshold values that were found by trail and error running these tests
    int firstGoodSpec = 36;
    int lastGoodSpec = 95;
    int savedBySignific = Nhist-1;
    for (int lHist = 1; lHist < firstGoodSpec; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), BadVal )
    }
    for (int lHist=firstGoodSpec; lHist <= lastGoodSpec; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), GoodVal )
    }
    for (int lHist = lastGoodSpec+1; lHist < savedBySignific; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), BadVal )
    }
    for (int lHist = savedBySignific; lHist < Nhist; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), GoodVal )
    }
    //now check the array
    std::vector<int>::const_iterator it = OArray.begin();
    for (int lHist = 0 ; lHist < firstGoodSpec; lHist++ )
    {
      TS_ASSERT_EQUALS( *it, lHist+1 )
      TS_ASSERT_THROWS_NOTHING( if ( it != OArray.end() ) ++it )
    }
    for (int lHist = lastGoodSpec+1 ; lHist < savedBySignific ; lHist++ )
    {
      TS_ASSERT_EQUALS( *it, lHist+1 )
      if ( it != OArray.end() ) ++it;
      else TS_ASSERT_EQUALS( lHist, savedBySignific - 1 );
    }
    //check that extra entries haven't been written to the array
    TS_ASSERT_EQUALS( it, OArray.end() )
  }

  void testFile()
  {
    MedianDetectorTest alg;
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( runInit(alg) ) )

    // values a little extreme, I just made them up
    alg.setProperty( "LowThreshold", 0.44444 );
    alg.setProperty( "HighThreshold", 5.0 );
    // this should turn off examining the errors.  This makes things simplier, significance testing was tested in the last test
    alg.setProperty("SignificanceTest", 0.0);

    const int fSpec = 0,  lSpec = Nhist/2;
    alg.setProperty( "StartWorkspaceIndex", fSpec );
    alg.setProperty( "EndWorkspaceIndex", lSpec );
    //a couple of random numbers in the range
    const double lRange = 4000, uRange = 10000;
    alg.setProperty( "RangeLower", lRange );
    alg.setProperty( "RangeUpper", uRange );

    std::string OFileName("MedianDetectorTestTestFile.txt");  
    alg.setPropertyValue( "OutputFile", OFileName );

    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    //test file output
    std::fstream testFile(OFileName.c_str(), std::ios::in);
    //the tests here are done as unhandled exceptions cxxtest will handle the exceptions and display a message but only after this function has been abandoned, which leaves the file undeleted so it can be viewed
    TS_ASSERT ( testFile )
    
    // comfirmed these values by following the debugger up to this point
    int firstGoodSpec = 16;
    int lastGoodSpec = 360;
    std::string fileLine = "";
    for (int iHist = -1 ; iHist < firstGoodSpec; iHist++ )
    {
      std::ostringstream correctLine;
      if ( iHist == -1 )
        correctLine << "Index Spectrum UDET(S)";
      else
      {
        correctLine << " Spectrum number " << iHist+1 << " is too low, ";
      correctLine << "detector IDs: " << iHist+1;
      }
        
      std::getline( testFile, fileLine );
        
      TS_ASSERT_EQUALS ( fileLine, correctLine.str() )
    }
    for (int iHist = lastGoodSpec+1 ; iHist < Nhist; iHist++ )
    {
      std::ostringstream correctLine;
      correctLine << " Spectrum with number " << iHist+1 << " is too high";
      correctLine << " detector IDs: " << iHist+1;
      
      std::getline( testFile, fileLine );

      TS_ASSERT_EQUALS ( fileLine, correctLine.str() )

    }
    testFile.close();
    remove(OFileName.c_str());
  }
    
  MedianDetectorTestTest() : m_IWSName("MedianDetectorTestInput"), BadVal(100.0), GoodVal(0.0)
  {
    using namespace Mantid;
    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",Nhist,11,10);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
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
      space2D->setX(j, x);
      boost::shared_ptr<MantidVec> spectrum( new MantidVec );
      //the spectravalues will be multiples of the random numbers above
      for ( int l = 0; l < specLength-1; ++l )
      {
        spectrum->push_back( j*yArray[l] );
      }
      boost::shared_ptr<MantidVec> errors = smallErrors;
      if ( j == Nhist-2 ) errors = almostBigEnough;
      if ( j == Nhist-1 ) errors = bigEnough;

      space2D->setData( j, spectrum, errors );
      // Just set the spectrum number to match the index
      space2D->getAxis(1)->spectraNo(j) = j+1;
      forSpecDetMap[j] = j+1;
    }

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(m_IWSName, space);
    
    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "../../../../Test/Instrument/INS_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", m_IWSName);
    loader.execute(); 

    space2D->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, Nhist);

    space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
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
  std::string m_IWSName;
  double m_YSum;
  enum { Nhist = 144 };
  //these values must match the values in DetectorEfficiencyVariation.h
  const double BadVal;
  const double GoodVal;
};

#endif /*WBVMEDIANTESTTEST_H_*/
