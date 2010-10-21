#ifndef LoadRawBin0Test123_H_
#define LoadRawBin0Test123_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRawBin0.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "Poco/Path.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadRawBin0Test : public CxxTest::TestSuite
{
public:

  LoadRawBin0Test()
  {
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/HET15869.raw").toString();
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
	
    outputSpace = "bin0";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 2584);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(99)) == (output2D->dataX(1734)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(673).size(), output2D->dataY(2111).size() );

    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(3)[0], 24.0);
    // Check that the error on that value is correct
	TS_ASSERT_EQUALS( output2D->dataE(2)[0], std::sqrt(output2D->dataY(2)[0]));
    
    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output2D->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT( ! output2D-> isDistribution() )

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA( output2D->run().getProtonCharge(), 171.0353, 0.0001 )
	AnalysisDataService::Instance().remove(outputSpace);
  }

 
  void testMultiPeriod()
  {  
	
	LoadRawBin0 loader5;
    loader5.initialize();
    loader5.setPropertyValue("Filename", "../../../../Test/AutoTestData/EVS13895.raw");
    loader5.setPropertyValue("OutputWorkspace", "multiperiod");
    //loader5.setPropertyValue("SpectrumList", "10,50,100,195");
    
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
      TS_ASSERT_EQUALS( outsptr->getNumberHistograms(), 198 )

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
    TS_ASSERT_EQUALS( outsptr1->dataX(1), outsptr5->dataX(1) )
    TS_ASSERT_EQUALS( outsptr1->dataX(1), outsptr6->dataX(1) )

   
    TS_ASSERT_EQUALS( outsptr1->getInstrument(), outsptr2->getInstrument() )
    TS_ASSERT_EQUALS( &(outsptr1->spectraMap()), &(outsptr2->spectraMap()) )
    TS_ASSERT_EQUALS( &(outsptr1->sample()), &(outsptr2->sample()))
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr2->run()))
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr3->run()) )
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr4->run()) )
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr5->run()) )
    TS_ASSERT_EQUALS( outsptr1->getInstrument(), outsptr6->getInstrument() )
    TS_ASSERT_EQUALS( &(outsptr1->spectraMap()), &(outsptr6->spectraMap()) )
    TS_ASSERT_EQUALS( &(outsptr1->sample()), &(outsptr6->sample()) )
    TS_ASSERT_DIFFERS( &(outsptr1->run()), &(outsptr6->run()) )

	itr1=wsNamevec.begin();
    for (;itr1!=wsNamevec.end();++itr1)
	{
		AnalysisDataService::Instance().remove(*itr);
	}
	
  }
  
private:
  LoadRawBin0 loader;
  std::string inputFile;
  std::string outputSpace;
};

#endif /*LoadRawBin0Test_H_*/
