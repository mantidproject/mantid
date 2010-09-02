#ifndef SAVEDASCTEST_H_
#define SAVEDASCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveDASC.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include <boost/lexical_cast.hpp>
#include <vector>
#include <fstream>

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class SaveDASCTest : public CxxTest::TestSuite
{
public:
  void testPetWorkspace()
  {
    SaveDASC grouper;

    TS_ASSERT_THROWS_NOTHING( grouper.initialize() )
    TS_ASSERT( grouper.isInitialized() )

    // Set up a small workspace for testing
    makeSmallWS();
    grouper.setPropertyValue("InputWorkspace", SMALL_WS_NAME);
    grouper.setPropertyValue("Filename", FILENAME);
    TS_ASSERT_THROWS_NOTHING(grouper.execute());
    TS_ASSERT( grouper.isExecuted() );

    MatrixWorkspace_const_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(SMALL_WS_NAME));
    
    // read in that file and compare with what is in the workspace
    std::ifstream testFile(FILENAME.c_str(), std::ios::in);
    TS_ASSERT ( testFile )
    
    std::string fileLine;
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "#Number of TOF points (x)" )
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, boost::lexical_cast<std::string>(NBINS))
      
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "#Number of spectra numbers (y)")
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, boost::lexical_cast<std::string>(NHISTS))
      
    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "# TOF values (x)" )
    for (int j = 0 ; j < NBINS; j++ )
    {
      std::getline( testFile, fileLine );
      std::string correct = boost::lexical_cast<std::string>((j+0.5)/1000.0);
      TS_ASSERT_EQUALS ( fileLine, correct.substr(0, fileLine.length()) )
    }

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "# spectra values (y)" )
    for (int iHist = 0 ; iHist < NHISTS; iHist++ )
    {
      std::getline( testFile, fileLine );
      TS_ASSERT_EQUALS ( fileLine, boost::lexical_cast<std::string>(iHist+1) )
    }

    std::getline( testFile, fileLine );
    TS_ASSERT_EQUALS ( fileLine, "# Group" )
    for (int iHist = 0; iHist < NHISTS; iHist++ )
    {
      for (int j = 0 ; j < NBINS; j++ )
      {
        std::getline( testFile, fileLine );
        TS_ASSERT_EQUALS ( fileLine,
          boost::lexical_cast<std::string>(WS->readY(iHist)[j]) + " "
          + boost::lexical_cast<std::string>(WS->readE(iHist)[j]))
      }
    }

    testFile.close();
    remove(FILENAME.c_str());

    AnalysisDataService::Instance().remove(SMALL_WS_NAME);
  }



private:
  enum constants { NHISTS = 6, NBINS = 4};
  static const std::string SMALL_WS_NAME, FILENAME;
    
  // Set up a small workspace for testing
  void makeSmallWS()
  {
    MatrixWorkspace_sptr space =
      WorkspaceFactory::Instance().create("Workspace2D", NHISTS, NBINS+1, NBINS);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    MantidVecPtr xs, errors, data[NHISTS];
    xs = rampXs();
    errors.access().resize(NBINS, 1.0);
    for (int j = 0; j < NHISTS; ++j)
    {
      space2D->setX(j,xs);
      data[j].access().resize(NBINS, j + 1);  // the y values will be different for each spectra (1+index_number) but the same for each bin
      space2D->setData(j, data[j], errors);
      space2D->getAxis(1)->spectraNo(j) = j+1;  // spectra numbers are also 1 + index_numbers because this is the tradition
    }
    // Register the workspace in the data service
    AnalysisDataService::Instance().add(SMALL_WS_NAME, space);
  }

MantidVecPtr rampXs()
{
  MantidVecPtr xs;
  xs.access().resize(NBINS+1);
  for (int i = 0; i < NBINS+1; i ++)
    xs.access()[i] = i/1000.0;
  return xs;
}

};

const std::string SaveDASCTest::SMALL_WS_NAME = "SaveDASCTest_temporary_workspace";
const std::string SaveDASCTest::FILENAME =  "SaveDASCTest_temporary_file.dasc";

#endif /*SAVEDASCTEST_H_*/
