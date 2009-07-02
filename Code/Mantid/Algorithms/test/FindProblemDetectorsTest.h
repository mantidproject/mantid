#ifndef FINDPROBLEMDETECTORSTEST_H_
#define FINDPROBLEMDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/FindProblemDetectors.h"
#include "MantidKernel/PhysicalConstants.h"
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

//these values must match the values in FindProblemDetectors
static int BadVal = 100;
static int GoodVal = 0;

class FindProblemDetectorsTest : public CxxTest::TestSuite
{
public:

  void testWithoutAngles()
  {
    FindProblemDetectors alg;
    //TS_ASSERT_EQUALS( alg.name(), "FindProblemDetectors" )
    TS_ASSERT_EQUALS( alg.version(), 1 )
    //the spectra were setup in the constructor and passed to our algorithm through this function
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( runInit(alg) ) )

    //these are realistic values that I just made up
    const double lowThres = 0.5,  highThres = 1.3333;
    alg.setProperty( "LowThreshold", lowThres );
    alg.setProperty( "HighThreshold", highThres );
    //we are using the defaults on StartSpectrum, EndSpectrum, Range_lower and Range_upper which is to use the whole spectrum

    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    std::vector<int> OArray;
    TS_ASSERT_THROWS_NOTHING( OArray = alg.getProperty( "FoundDead" ) )
    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("FindProbDetectsTestOutput"));
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(m_IWSName));
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("FindProbDetectsTestOutput"));
    MatrixWorkspace_sptr outputMat = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    if ( !outputMat ) throw std::logic_error("outputMat is null or not a Matrix workspace, aborting some asserts");
    TS_ASSERT_EQUALS( outputMat->YUnit(), "" )

    //There are three output, a workspace (tested in the next code block), an array (later, this test) and a file (next test)
    //were all the spectra output?
		const int numberOfSpectra = outputMat->getNumberHistograms();
    TS_ASSERT_EQUALS(numberOfSpectra, (int)Nhist);
    //the number of counts is proportional to spectra number (no solid angle calculation to disturb things) so the number that are below the low limit is the number below the median (half of them) x threshold fraction
    int firstGoodSpec = static_cast<int>(ceil(lowThres*Nhist/2.0));//the factor of 2 here is because the threshold is related to the median
    int lastGoodSpec = static_cast<int>(floor(highThres*Nhist/2.0));
    for (int lHist = 0; lHist < firstGoodSpec; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), BadVal )
    }
    for (int lHist=firstGoodSpec; lHist <= lastGoodSpec; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), GoodVal )
    }
    for (int lHist = lastGoodSpec+1; lHist < Nhist; lHist++)
    {
      TS_ASSERT_EQUALS(
        outputMat->readY(lHist).front(), BadVal )
    }
    //now check the array
    std::vector<int>::const_iterator it = OArray.begin();
    for (int lHist = 0 ; lHist < firstGoodSpec; lHist++ )
    {// for each spectrum we have linked two detectors, check that both of those are listed
      TS_ASSERT_EQUALS( *it, lHist*2 )
      TS_ASSERT_THROWS_NOTHING( ++it )
      TS_ASSERT_EQUALS( *it, lHist*2+1 )
      TS_ASSERT_THROWS_NOTHING( 
        if ( it != OArray.end() ) ++it )
    }
    //check that extra entries haven't been written to the array
    TS_ASSERT( it != OArray.end() )
  }

  void testWithSolidAngles()
  {/* STEVE activate this test
    TS_ASSERT_THROWS_NOTHING( addInstrumentGeom() )
    
//    // Mark one detector dead to test that it leads to zero solid angle
//    Detector* det143 = dynamic_cast<Detector*>(input2D->getDetector(143).get());
//    boost::shared_ptr<ParameterMap> pmap = input2D->instrumentParameters();
//    pmap->addBool(det143,"masked",true);

    FindProblemDetectors alg;
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( runInit(alg) ) )

    //these are realistic values that I just made up
    const double lowThres = 0.44444,  highThres = 5;
    alg.setProperty( "LowThreshold", lowThres );
    alg.setProperty( "HighThreshold", highThres );

    const int fSpec = 0,  lSpec = Nhist/2;
    alg.setProperty( "StartSpectrum", fSpec );
    alg.setProperty( "EndSpectrum", lSpec );
    //a couple of random numbers in the range
    const double lRang = 4000, uRange = 10000;
    alg.setProperty( "Range_lower", lRang );
    alg.setProperty( "Range_upper", uRange );

    std::string OFileName("FindProbDetectsTestFile.txt");  
    alg.setPropertyValue( "OutputFile", OFileName );

    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    //test file output
    std::fstream testFile(OFileName.c_str(), std::ios::in);
    //the tests here are done as unhandled exceptions cxxtest will handle the exceptions and display a message but only after this function has been abandoned, which leaves the file undeleted so it can be viewed
    if ( !testFile )
      throw std::runtime_error("Could not open the file " + OFileName);
    
    int firstGoodSpec = static_cast<int>(ceil(lowThres*Nhist/2.0));//the factor of 2 here is because the threshold is related to the median
    int lastGoodSpec = static_cast<int>(floor(highThres*Nhist/2.0));
    std::string fileLine = "";
    for (int numHist = -1 ; numHist < firstGoodSpec; numHist++ )
    {
      std::ostringstream correctLine;
      if ( numHist == -1 )
        correctLine << "Index Spectrum UDET(S)";
      else
      {
        correctLine << " Spectrum " << numHist << " is too low";
        correctLine << " detector IDs: " << numHist*2 << ", " << numHist*2+1;
        correctLine << std::endl;
      }
        
      std::getline( testFile, fileLine );
        
      if ( testFile.rdstate() & std::ios::failbit )
      {
        throw std::exception(
          "File incomplete, end of file found while reading the low reading detector list");
      }
      if ( fileLine != correctLine.str() )
      {
        throw std::runtime_error( "Output file testing: problem on line " +
          boost::lexical_cast<std::string>(numHist+1) + " of file " + OFileName );
      }
    }
    for (int numHist = lastGoodSpec+1 ; numHist < Nhist; numHist++ )
    {
      std::ostringstream correctLine;
      correctLine << " Spectrum " << numHist << " is too high";
      correctLine << " detector IDs: " << numHist*2 << ", " << numHist*2+1;
      correctLine << std::endl;
      
      std::getline( testFile, fileLine );

      if ( testFile.rdstate() & std::ios::failbit )
      {
        throw std::exception(
          "File incomplete, end of file found while reading the high reading detector list");
      }
      if ( fileLine != correctLine.str() )
      {
        throw std::runtime_error( "Problem on line " +
          boost::lexical_cast<std::string>(numHist) + " " + OFileName );
      }
    }
    testFile.close();
    remove(OFileName.c_str());*/
  }
    
  FindProblemDetectorsTest() : m_IWSName("FindProbDetectsTestInput")
  {
    using namespace Mantid;
    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",Nhist,11,10);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    boost::shared_ptr<MantidVec> x(new MantidVec(11));
    for (int i = 0; i < 11; ++i)
    {
      (*x)[i]=i*1000;
    }
    // 21 random numbers that will be copied into the workspace spectra
    const short ySize = 21;
    double yArray[ySize] =
      {0.2,4,50,0.001,0,0,0,1,0,15,4,0,0.001,2e-10,0,8,0,1e-4,1,7,11};
    m_YSum = 0; for (int i = 0; i < ySize; i++) m_YSum += yArray[i];

    //the error values aren't used and aren't tested so we'll use some basic data
    boost::shared_ptr<MantidVec> errors( new MantidVec( ySize, 1) );
    boost::shared_ptr<MantidVec> spectrum;

    for (int j = 0; j < Nhist; ++j)
    {
      space2D->setX(j, x);
      spectrum.reset( new MantidVec );
      //the spectrums are multiples of the random numbers above
      for ( int l = 0; l < ySize; ++l )
      {
        spectrum->push_back( j*yArray[l] );
      }
      space2D->setData( j, spectrum, errors );
      // Just set the spectrum number to match the index
      space2D->getAxis(1)->spectraNo(j) = j;
    }
    
    // Populate the spectraDetectorMap with fake data, each spectrum will have two detectors
    std::vector<int> udetList(2);
    for ( int j = 0; j < Nhist; j++ )
    {
      udetList[0] = 2*j;
      udetList[1] = 2*j+1;
      space2D->mutableSpectraMap().addSpectrumEntries( j, udetList);
    }

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(m_IWSName, space);
  }

  bool runInit(FindProblemDetectors &alg)
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    bool good = alg.isInitialized();

    // Set the properties
    alg.setPropertyValue("WhiteBeamWorkspace", m_IWSName);
    alg.setPropertyValue("OutputWorkspace","FindProbDetectsTestOutput");
    return good;
  }

  void addInstrumentGeom()
  {
    // Get input workspace back to add detector geometry info to
    Workspace_sptr input;
    input = AnalysisDataService::Instance().retrieve(m_IWSName);
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(input);
   
    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "../../../../Test/Instrument/INS_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", m_IWSName);
    loader.execute();
  }
 
private:
  std::string m_IWSName;
  double m_YSum;
  enum { Nhist = 144 };
};

#endif /*FINDPROBLEMDETECTORSTEST_H_*/
