#ifndef LoadRawSpectrum0Test_H_
#define LoadRawSpectrum0Test_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRawSpectrum0.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadRawSpectrum0Test : public CxxTest::TestSuite
{
public:

  static LoadRawSpectrum0Test *createSuite() { return new LoadRawSpectrum0Test(); }
  static void destroySuite(LoadRawSpectrum0Test *suite) { delete suite; }

  LoadRawSpectrum0Test()
  {
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "HET15869.raw";
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized() );
  }

  void testExec()
  {
	/* std::string s;
	 std::getline(std::cin,s);*/
	  if ( !loader.isInitialized() ) loader.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loader.execute(),std::runtime_error);

    // Now set it...
    loader.setPropertyValue("Filename", inputFile);
	
    outputSpace = "outer";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 1);
    // Check two X vectors are the same
   
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(0)[777], 355);
    // Check that the error on that value is correct
	TS_ASSERT_EQUALS( output2D->dataE(0)[777],std::sqrt(output2D->dataY(0)[777]));
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataX(0)[777], 554.1875);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output2D->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT( ! output2D-> isDistribution() )

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA( output2D->run().getProtonCharge(), 171.0353, 0.0001 )
	AnalysisDataService::Instance().remove(outputSpace);
  }

 
  void testMultiPeriod()
  {
	LoadRawSpectrum0 loader5;
    loader5.initialize();
    loader5.setPropertyValue("Filename", "EVS13895.raw");
    loader5.setPropertyValue("OutputWorkspace", "multiperiod");
     
    TS_ASSERT_THROWS_NOTHING( loader5.execute() )
    TS_ASSERT( loader5.isExecuted() )
	
    WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(work_out = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("multiperiod")));
	
    Workspace_sptr wsSptr=AnalysisDataService::Instance().retrieve("multiperiod");
    WorkspaceGroup_sptr sptrWSGrp=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
    std::vector<std::string>wsNamevec;
    wsNamevec=sptrWSGrp->getNames();
    int period=1;
    std::vector<std::string>::const_iterator it=wsNamevec.begin();
    for (;it!=wsNamevec.end();it++)
    {	std::stringstream count;
      count <<period;
      std::string wsName="multiperiod_"+count.str();
      TS_ASSERT_EQUALS(*it,wsName)
      period++;
    }
    std::vector<std::string>::const_iterator itr1=wsNamevec.begin();
    for (;itr1!=wsNamevec.end();itr1++)
    {	
      MatrixWorkspace_sptr  outsptr;
      TS_ASSERT_THROWS_NOTHING(outsptr=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*itr1))));
      TS_ASSERT_EQUALS( outsptr->getNumberHistograms(),1 )

    }
    std::vector<std::string>::const_iterator itr=wsNamevec.begin();

    MatrixWorkspace_sptr  outsptr1;
    TS_ASSERT_THROWS_NOTHING(outsptr1=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*itr))));
    MatrixWorkspace_sptr  outsptr2;
    TS_ASSERT_THROWS_NOTHING(outsptr2=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*++itr))));
    MatrixWorkspace_sptr  outsptr3;
    TS_ASSERT_THROWS_NOTHING(outsptr3=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*++itr))));
    MatrixWorkspace_sptr  outsptr4;
    TS_ASSERT_THROWS_NOTHING(outsptr4=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*++itr))));
    MatrixWorkspace_sptr  outsptr5;
    TS_ASSERT_THROWS_NOTHING(outsptr5=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*++itr))));
    MatrixWorkspace_sptr  outsptr6;
    TS_ASSERT_THROWS_NOTHING(outsptr6=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*++itr))));
		
    TS_ASSERT_EQUALS( outsptr1->dataX(0), outsptr2->dataX(0) )
    TS_ASSERT_EQUALS( outsptr1->dataX(0), outsptr3->dataX(0) )
    TS_ASSERT_EQUALS( outsptr1->dataX(0), outsptr4->dataX(0) )
    TS_ASSERT_EQUALS( outsptr1->dataX(0), outsptr5->dataX(0) )
    TS_ASSERT_EQUALS( outsptr1->dataX(0), outsptr6->dataX(0) )

    // But the data should be different
    TS_ASSERT_DIFFERS( outsptr1->dataY(0)[555], outsptr2->dataY(0)[555] )
    TS_ASSERT_DIFFERS( outsptr1->dataY(0)[555], outsptr3->dataY(0)[555] )
    TS_ASSERT_DIFFERS( outsptr1->dataY(0)[555], outsptr4->dataY(0)[555] )
    TS_ASSERT_DIFFERS( outsptr1->dataY(0)[555], outsptr5->dataY(0)[555] )
    TS_ASSERT_DIFFERS( outsptr1->dataY(0)[555], outsptr6->dataY(0)[555] )

    TS_ASSERT_EQUALS( outsptr1->getBaseInstrument(), outsptr2->getBaseInstrument() )
    TS_ASSERT_EQUALS( &(outsptr1->spectraMap()), &(outsptr2->spectraMap()) )
    TS_ASSERT_EQUALS( &(outsptr1->sample()), &(outsptr2->sample()))
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr2->run()))
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr3->run()) )
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr4->run()) )
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr5->run()) )
    TS_ASSERT_EQUALS( outsptr1->getBaseInstrument(), outsptr6->getBaseInstrument() )
    TS_ASSERT_EQUALS( &(outsptr1->spectraMap()), &(outsptr6->spectraMap()) )
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr6->run()) )
	
  }
  
private:
  LoadRawSpectrum0 loader;
  std::string inputFile;
  std::string outputSpace;
};

#endif /*LoadRawSpectrum0Test_H_*/
