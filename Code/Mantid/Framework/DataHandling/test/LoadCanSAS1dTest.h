#ifndef LOADCANSAS1DTEST_H
#define LOADCANSAS1DTEST_H

//------------------------------------------------
// Includes
//------------------------------------------------

#include <cxxtest/TestSuite.h>

#include"MantidDataHandling/LoadCanSAS1D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "Poco/Path.h"

class LoadCanSAS1dTest : public CxxTest::TestSuite
{
public:
	LoadCanSAS1dTest()
	{
		 inputFile = Poco::Path(Poco::Path::current()).resolve("../../../../Test/AutoTestData/LOQ_CANSAS1D.xml").toString();
	}
	 void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( cansas1d.initialize());
    TS_ASSERT( cansas1d.isInitialized() );
  }

  void testSingleEntry()
  {
	  if ( !cansas1d.isInitialized() ) cansas1d.initialize();

    //No parameters have been set yet, so it should throw
    TS_ASSERT_THROWS(cansas1d.execute(), std::runtime_error);

    //Set the file name
    cansas1d.setPropertyValue("Filename", inputFile);
    
    std::string outputSpace = "outws";
    //Set an output workspace
    cansas1d.setPropertyValue("OutputWorkspace", outputSpace);
    
    //check that retrieving the filename gets the correct value
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = cansas1d.getPropertyValue("Filename") )
    TS_ASSERT( result.compare(inputFile) == 0 );

    TS_ASSERT_THROWS_NOTHING( result = cansas1d.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( result == outputSpace );

    //Should now throw nothing
    TS_ASSERT_THROWS_NOTHING( cansas1d.execute() );
    TS_ASSERT( cansas1d.isExecuted() );

	 //Now need to test the resultant workspace, first retrieve it
	Mantid::API::Workspace_sptr ws;
	TS_ASSERT_THROWS_NOTHING( ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace) );
	Mantid::DataObjects::Workspace2D_sptr ws2d = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);

  Mantid::Kernel::Property *logP = ws2d->run().getLogData("run_number");
  TS_ASSERT_EQUALS( logP->value(), "LOQ48097")

  //Single histogram
   TS_ASSERT_EQUALS( ws2d->getNumberHistograms(), 1 );

   //Test the size of the data vectors (there should be 102 data points so x have 103)
    TS_ASSERT_EQUALS( (ws2d->dataX(0).size()), 102);
    TS_ASSERT_EQUALS( (ws2d->dataY(0).size()), 102);
    TS_ASSERT_EQUALS( (ws2d->dataE(0).size()), 102);


    double tolerance(1e-06);
    TS_ASSERT_DELTA( ws2d->dataX(0)[0], 0.0604703, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[1], 0.0620232, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[2], 0.0635737, tolerance );
    ////Test a couple of random ones
    TS_ASSERT_DELTA( ws2d->dataX(0)[20], 0.0991537, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[64], 0.293873, tolerance );
    
   
    TS_ASSERT_DELTA( ws2d->dataX(0)[100], 0.714858, tolerance );
    TS_ASSERT_DELTA( ws2d->dataX(0)[101], 0.732729, tolerance );


    TS_ASSERT_DELTA( ws2d->dataY(0)[0], 12, tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(0)[25], 4674, tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(0)[99], 1, tolerance );


    TS_ASSERT_DELTA( ws2d->dataE(0)[0], 3.4641, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(0)[25], 68.3667, tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(0)[99], 1, tolerance );
     
  }

  void testMultipleEntries()
  {
    using namespace std;
    using namespace Mantid;
    using namespace API;
    using namespace DataObjects;
    using namespace DataHandling;

    LoadCanSAS1D alg;
    alg.initialize();

    std::string outputSpace = "LoadCanSAS1DTest_out";
    //Set an output workspace
    alg.setPropertyValue("OutputWorkspace", outputSpace); 
    alg.setPropertyValue("Filename", "../../../../Test/AutoTestData/testCansas1DMultiEntry.xml");

    TS_ASSERT_THROWS_NOTHING( alg.execute() )
    TS_ASSERT( alg.isExecuted() );
    
    //Now need to test the resultant workspace, first retrieve it
    Workspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieve(outputSpace);
    WorkspaceGroup_sptr group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
    TS_ASSERT(group)
    vector<string> wNames = group->getNames();
    
    TS_ASSERT_EQUALS(wNames.size(),2)//change this and the lines below when group workspace names change

    ws = Mantid::API::AnalysisDataService::Instance().retrieve(wNames[0]);
	Mantid::DataObjects::Workspace2D_sptr ws2d = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);
  TS_ASSERT(ws2d)

  Run run = ws2d->run();
  Mantid::Kernel::Property *logP =run.getLogData("run_number");
  TS_ASSERT_EQUALS( logP->value(), "53616")
  TS_ASSERT_EQUALS(run.getLogData("UserFile")->value(), "MASK.094AA");
  TS_ASSERT_EQUALS(ws2d->getInstrument()->getName(), "LOQ")

  TS_ASSERT_EQUALS( ws2d->getNumberHistograms(), 1 );
  TS_ASSERT_EQUALS( ws2d->dataX(0).size(), 143 );

  //some of the data is only stored to 3 decimal places
  double tolerance(1e-04);
  //this tests just the first, a middle and last value the other worksapce has all its values check below
  const int testIndices[] = {0, 70, 142};
  for (int i = 0; i < 3; ++i)
  {
    TS_ASSERT_DELTA( ws2d->dataX(0)[testIndices[i]], xs99631[i], tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(0)[testIndices[i]], ys99631[i], tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(0)[testIndices[i]], es99631[i], tolerance );
  }

    ws = Mantid::API::AnalysisDataService::Instance().retrieve(wNames[1]);
	ws2d = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws);
  TS_ASSERT(ws2d)

  run = ws2d->run();
  logP =run.getLogData("run_number");
  TS_ASSERT_EQUALS( logP->value(), "808")
  TS_ASSERT_EQUALS(run.getLogData("UserFile")->value(), "MASKSANS2D.091A");
  TS_ASSERT_EQUALS(ws2d->getInstrument()->getName(), "SANS2D")

  TS_ASSERT_EQUALS( ws2d->getNumberHistograms(), 1 );
  TS_ASSERT_EQUALS( ws2d->dataX(0).size(), 23 );

  //testing all workspace data, as there isn't much
  for (int i = 0; i < 23; ++i)
  {
    TS_ASSERT_DELTA( ws2d->dataX(0)[i], xs808[i], tolerance );
    TS_ASSERT_DELTA( ws2d->dataY(0)[i], ys808[i], tolerance );
    TS_ASSERT_DELTA( ws2d->dataE(0)[i], es808[i], tolerance );
  }

}
private:
	std::string inputFile;
	Mantid::DataHandling::LoadCanSAS1D cansas1d;

  static const double xs99631[3], ys99631[3], es99631[3];
  static const double xs808[23], ys808[23], es808[23];
};
const double LoadCanSAS1dTest::xs99631[] = {0.0109, 0.151, 0.2949};
const double LoadCanSAS1dTest::ys99631[] = {5.44952, 0.15223, 0.14831};
const double LoadCanSAS1dTest::es99631[] = {0.0588457, 0.0043596, 0.335294};

const double LoadCanSAS1dTest::xs808[] = {0.646222,       0.659146,       0.672329,       0.685775,       0.699491,       0.713481,        0.72775,       0.742305,       0.757152,       0.772295,        0.78774,       0.803495,       0.819565,       0.835956,       0.852676,       0.869729,       0.887124,       0.904866,       0.922963,       0.941423,       0.960251,       0.979456,       0.994577};
const double LoadCanSAS1dTest::ys808[] = {5.59202,        5.27307,        4.78682,        4.66635,        4.82897,        5.05591,        4.58635,        4.15975,        4.01298,         4.1226,        4.04966,        3.90263,         3.4256,        3.29929,        3.17003,        2.67487,        2.41979,        2.31446,        2.18734,        2.11788,        2.03716,        2.03615,        2.01552};
const double LoadCanSAS1dTest::es808[] = {0.219459,       0.203702,       0.186871,       0.178849,       0.172545,        0.17094,       0.153269,       0.141219,        0.13289,       0.130725,       0.123281,        0.11705,       0.104102,      0.0991949,      0.0933884,       0.082556,      0.0757769,      0.0715806,      0.0674828,       0.064006,      0.0600373,      0.0581645,      0.0766164};

#endif
