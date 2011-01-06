#ifndef CROPWORKSPACETEST_H_
#define CROPWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class CropWorkspaceTest : public CxxTest::TestSuite
{
public:
  CropWorkspaceTest()
  {
    // Set up a small workspace for testing
    Workspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",5,6,5);
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    double *a = new double[25];
    double *e = new double[25];
    for (int i = 0; i < 25; ++i)
    {
      a[i]=i;
      e[i]=sqrt(double(i));
    }
    for (int j = 0; j < 5; ++j) {
      for (int k = 0; k < 6; ++k) {
        space2D->dataX(j)[k] = k;
      }
      space2D->setData(j, boost::shared_ptr<Mantid::MantidVec>(new std::vector<double>(a+(5*j), a+(5*j)+5)),
          boost::shared_ptr<Mantid::MantidVec>(new std::vector<double>(e+(5*j), e+(5*j)+5)));
      space2D->getAxis(1)->spectraNo(j) = j;
    }
    // Register the workspace in the data service
    AnalysisDataService::Instance().add("toCrop", space);
  }

  void testName()
  {
    TS_ASSERT_EQUALS( crop.name(), "CropWorkspace" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( crop.version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( crop.category(), "General" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( crop.initialize() );
    TS_ASSERT( crop.isInitialized() );
  }

  void testInvalidInputs()
  {
    if ( !crop.isInitialized() ) crop.initialize();

    TS_ASSERT_THROWS( crop.execute(), std::runtime_error );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("InputWorkspace","toCrop") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("OutputWorkspace","nothing") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMin","2") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMax","1") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMax","2.5") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMax","5") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("StartWorkspaceIndex","10") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("StartWorkspaceIndex","4") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("EndWorkspaceIndex","10") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("EndWorkspaceIndex","2") );
    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( !crop.isExecuted() );
  }

  void testExec()
  {
    if ( !crop.isInitialized() ) crop.initialize();

    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("InputWorkspace","toCrop") );
    std::string outputWS("cropped");
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMin","0.1") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("XMax","4") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("StartWorkspaceIndex","2") );
    TS_ASSERT_THROWS_NOTHING( crop.setPropertyValue("EndWorkspaceIndex","4") );

    TS_ASSERT_THROWS_NOTHING( crop.execute() );
    TS_ASSERT( crop.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) );

    TS_ASSERT_EQUALS( output->getNumberHistograms(), 3 );
    TS_ASSERT_EQUALS( output->blocksize(), 3 );

    MatrixWorkspace_const_sptr input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("toCrop"));
    for (int i=0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        TS_ASSERT_EQUALS( output->readX(i)[j], input->readX(i+2)[j+1] );
        TS_ASSERT_EQUALS( output->readY(i)[j], input->readY(i+2)[j+1] );
        TS_ASSERT_EQUALS( output->readE(i)[j], input->readE(i+2)[j+1] );
      }
      TS_ASSERT_EQUALS( output->readX(i)[3], input->readX(i+2)[4] );
      TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(i), input->getAxis(1)->spectraNo(i+2) );
    }
  }

  void testExecWithDefaults()
  {
	  CropWorkspace crop2;
	  TS_ASSERT_THROWS_NOTHING( crop2.initialize() );
    TS_ASSERT_THROWS_NOTHING( crop2.setPropertyValue("InputWorkspace","toCrop") );
    TS_ASSERT_THROWS_NOTHING( crop2.setPropertyValue("OutputWorkspace","unCropped") );
    TS_ASSERT_THROWS_NOTHING( crop2.execute() );
    TS_ASSERT( crop2.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("unCropped")) );
    MatrixWorkspace_const_sptr input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("toCrop"));

    MatrixWorkspace::const_iterator inIt(*input);
    for (MatrixWorkspace::const_iterator it(*output); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), inIt->X() );
      TS_ASSERT_EQUALS( it->X2(), inIt->X2() );
      TS_ASSERT_EQUALS( it->Y(), inIt->Y() );
      TS_ASSERT_EQUALS( it->E(), inIt->E() );
    }
    for (int i = 0; i < 5; ++i)
    {
      TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(i), input->getAxis(1)->spectraNo(i) );
    }
  }

  void testWithPointData()
  {
    AnalysisDataService::Instance().add("point",WorkspaceCreationHelper::Create2DWorkspace123(5,5));
    CropWorkspace crop3;
    TS_ASSERT_THROWS_NOTHING( crop3.initialize() );
    TS_ASSERT_THROWS_NOTHING( crop3.setPropertyValue("InputWorkspace","point") );
    TS_ASSERT_THROWS_NOTHING( crop3.setPropertyValue("OutputWorkspace","pointOut") );
    TS_ASSERT_THROWS_NOTHING( crop3.execute() );
    TS_ASSERT( crop3.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("pointOut")) );
    MatrixWorkspace_const_sptr input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("point"));

    MatrixWorkspace::const_iterator inIt(*input);
    for (MatrixWorkspace::const_iterator it(*output); it != it.end(); ++it,++inIt)
    {
      TS_ASSERT_EQUALS( it->X(), inIt->X() );
      TS_ASSERT_THROWS( it->X2(), Mantid::Kernel::Exception::NotFoundError );
      TS_ASSERT_EQUALS( it->Y(), inIt->Y() );
      TS_ASSERT_EQUALS( it->E(), inIt->E() );
    }
  }

  void testRagged()
  {
    MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("toCrop"));
    // Change the first X vector
    for (int k = 0; k < 6; ++k) {
      input->dataX(0)[k] = k+3;
    }

    CropWorkspace crop4;
    TS_ASSERT_THROWS_NOTHING( crop4.initialize() );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("InputWorkspace","toCrop") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("OutputWorkspace","raggedOut") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMin","2.9") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMax","4.1") );
    TS_ASSERT_THROWS_NOTHING( crop4.execute() );
    TS_ASSERT( crop4.isExecuted() );
    
    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("raggedOut")) );

    TS_ASSERT_EQUALS( output->size(), input->size() );

    for (int i = 0; i < 5; ++i)
    {
      for (int j = 0; j < 5; ++j)
      {
        if ( (i == 0 && j == 0) || (i != 0 && j == 3) )
        {
          TS_ASSERT_EQUALS( output->readY(i)[j], input->readY(i)[j] );
        }
        else
        {
          TS_ASSERT_EQUALS( output->readY(i)[j], 0.0 );
        }
      }
    }

  }

  void testNegativeBinBoundaries()
  {
    const std::string wsName("neg");
    AnalysisDataService::Instance().add(wsName,WorkspaceCreationHelper::Create2DWorkspaceBinned(1,5,-6));
    CropWorkspace crop4;
    TS_ASSERT_THROWS_NOTHING( crop4.initialize() );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("InputWorkspace",wsName) );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("OutputWorkspace",wsName) );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMin","-5") );
    TS_ASSERT_THROWS_NOTHING( crop4.setPropertyValue("XMax","-2") );
    TS_ASSERT( crop4.execute() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName)) );

    TSM_ASSERT_EQUALS( "The number of bins", 3, output->blocksize() );
    TSM_ASSERT_EQUALS( "First bin boundary", -5, output->readX(0).front() );
    TSM_ASSERT_EQUALS( "Last bin boundary", -2, output->readX(0).back() );

    AnalysisDataService::Instance().remove(wsName);
  }

private:
  CropWorkspace crop;
};

#endif /*CROPWORKSPACETEST_H_*/
