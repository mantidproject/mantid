#ifndef LoadRaw3TEST_H_
#define LoadRaw3TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/Instrument.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadRaw3Test : public CxxTest::TestSuite
{
public:

  LoadRaw3Test()
  {
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HET15869.RAW";
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized() );
  }

  void testExec()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loader.execute(),std::runtime_error);

    // Now set it...
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "outer";
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
    TS_ASSERT_EQUALS( output2D->dataY(999)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(999)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataX(999)[777], 554.1875);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output2D->getAxis(0)->unit()->unitID(), "TOF" )
    TS_ASSERT( ! output2D-> isDistribution() )

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA( output2D->getSample()->getProtonCharge(), 171.0353, 0.0001 )

    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    boost::shared_ptr<IInstrument> i = output2D->getInstrument();
    boost::shared_ptr<Mantid::Geometry::IComponent> source = i->getSource();

    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<Mantid::Geometry::IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Z(), 0.0,0.01);

    boost::shared_ptr<Mantid::Geometry::Detector> ptrDet103 = boost::dynamic_pointer_cast<Mantid::Geometry::Detector>(i->getDetector(103));
    TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
    TS_ASSERT_EQUALS( ptrDet103->getName(), "pixel");
    TS_ASSERT_DELTA( ptrDet103->getPos().X(), 0.4013,0.01);
    TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 2.4470,0.01);

    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check sub-algorithm is running properly
    //----------------------------------------------------------------------
    boost::shared_ptr<Sample> sample = output2D->getSample();
    const std::vector< Property* >& pro = sample->getLogData();
    Property *l_property = sample->getLogData( std::string("TEMP1") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,23), "2007-Nov-13 15:16:20  0" );

    //----------------------------------------------------------------------
    // Tests to check that Loading SpectraDetectorMap is done correctly
    //----------------------------------------------------------------------
    const SpectraDetectorMap& map= output2D->spectraMap();

    // Check the total number of elements in the map for HET
    TS_ASSERT_EQUALS(map.nElements(),24964);

    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(map.ndet(6),1);

    // Test one to many mapping, for example 10 pixels contribute to spectra 2084
    TS_ASSERT_EQUALS(map.ndet(2084),10);
    // Check the id number of all pixels contributing
    std::vector<int> detectorgroup;
    detectorgroup=map.getDetectors(2084);
    std::vector<int>::const_iterator it;
    int pixnum=101191;
    for (it=detectorgroup.begin();it!=detectorgroup.end();it++)
    TS_ASSERT_EQUALS(*it,pixnum++);

    // Test with spectra that does not exist
    // Test that number of pixel=0
    TS_ASSERT_EQUALS(map.ndet(5),0);
    // Test that trying to get the Detector throws.
    std::vector<int> test = map.getDetectors(5);
    TS_ASSERT(test.empty());
  }

  void testMixedLimits()
  {
    if ( !loader2.isInitialized() ) loader2.initialize();

    loader2.setPropertyValue("Filename", inputFile);
    loader2.setPropertyValue("OutputWorkspace", "outWS");
    loader2.setPropertyValue("SpectrumList", "998,999,1000");
    loader2.setPropertyValue("SpectrumMin", "5");
    loader2.setPropertyValue("SpectrumMax", "10");

    TS_ASSERT_THROWS_NOTHING(loader2.execute());
    TS_ASSERT( loader2.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

    // Should be 6 for selected input
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 9);

    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(1)) == (output2D->dataX(5)) );

    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(2).size(), output2D->dataY(7).size() );

    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(8)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(8)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataX(8)[777], 554.1875);
  }

  void testMinlimit()
  {
    LoadRaw3 alg;
    std::string outWS = "outWSLimitTest";
    if ( !alg.isInitialized() ) alg.initialize();

    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", outWS);
    alg.setPropertyValue("SpectrumMin", "2580");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 5);
    AnalysisDataService::Instance().remove(outWS);
  }

  void testMaxlimit()
  {
    LoadRaw3 alg;
    std::string outWS = "outWSLimitTest";
    if ( !alg.isInitialized() ) alg.initialize();

    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", outWS);
    alg.setPropertyValue("SpectrumMax", "5");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 5);
    AnalysisDataService::Instance().remove(outWS);
  }

  void testMinMaxlimit()
  {
    LoadRaw3 alg;
    std::string outWS = "outWSLimitTest";
    if ( !alg.isInitialized() ) alg.initialize();

    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", outWS);
    alg.setPropertyValue("SpectrumMin", "5");
    alg.setPropertyValue("SpectrumMax", "10");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 6);
    AnalysisDataService::Instance().remove(outWS);
  }

  void testListlimit()
  {
    LoadRaw3 alg;
    std::string outWS = "outWSLimitTest";
    if ( !alg.isInitialized() ) alg.initialize();

    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", outWS);
    alg.setPropertyValue("SpectrumList", "998,999,1000");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 3);
    AnalysisDataService::Instance().remove(outWS);
  }

  void testfail()
  {
    if ( !loader3.isInitialized() ) loader3.initialize();
    std::string outWS="LoadRaw3-out2";
    loader3.setPropertyValue("Filename", inputFile);
    loader3.setPropertyValue("OutputWorkspace",outWS );
    loader3.setPropertyValue("SpectrumList", "0,999,1000");
    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "10");
    loader3.execute();
    Workspace_sptr output;
    // test that there is no workspace as it should have failed
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "1");
     loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "3");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "5");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "3000");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS),std::runtime_error);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "10");
    loader3.setPropertyValue("SpectrumList", "999,3000");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS),std::runtime_error);

    loader3.setPropertyValue("SpectrumList", "999,2000");
    loader3.execute();
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
  }

  void testMultiPeriod()
  {
    LoadRaw3 loader5;
    loader5.initialize();
    loader5.setPropertyValue("Filename", "../../../../Test/Data/EVS13895.raw");
    loader5.setPropertyValue("OutputWorkspace", "multiperiod");
    loader5.setPropertyValue("SpectrumList", "10,50,100,195");
    
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
    for (it++;it!=wsNamevec.end();it++)
    {	std::stringstream count;
      count <<period;
      std::string wsName="multiperiod_"+count.str();
      TS_ASSERT_EQUALS(*it,wsName)
      period++;
    }
    std::vector<std::string>::const_iterator itr1=wsNamevec.begin();
    for (itr1++;itr1!=wsNamevec.end();itr1++)
    {	
      MatrixWorkspace_sptr  outsptr;
      TS_ASSERT_THROWS_NOTHING(outsptr=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*itr1))));
      TS_ASSERT_EQUALS( outsptr->getNumberHistograms(), 4 )

    }
    std::vector<std::string>::const_iterator itr=wsNamevec.begin();
    itr++;
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

    // But the data should be different
    TS_ASSERT_DIFFERS( outsptr1->dataY(1)[555], outsptr2->dataY(1)[555] )
    TS_ASSERT_DIFFERS( outsptr1->dataY(1)[555], outsptr3->dataY(1)[555] )
    TS_ASSERT_DIFFERS( outsptr1->dataY(1)[555], outsptr4->dataY(1)[555] )
    TS_ASSERT_DIFFERS( outsptr1->dataY(1)[555], outsptr5->dataY(1)[555] )
    TS_ASSERT_DIFFERS( outsptr1->dataY(1)[555], outsptr6->dataY(1)[555] )

    TS_ASSERT_EQUALS( outsptr1->getInstrument(), outsptr2->getInstrument() )
    TS_ASSERT_EQUALS( &(outsptr1->spectraMap()), &(outsptr2->spectraMap()) )
    TS_ASSERT_DIFFERS( outsptr1->getSample(), outsptr2->getSample() )
    TS_ASSERT_DIFFERS( outsptr1->getSample(), outsptr3->getSample() )
    TS_ASSERT_DIFFERS( outsptr1->getSample(), outsptr4->getSample() )
    TS_ASSERT_DIFFERS( outsptr1->getSample(), outsptr5->getSample() )
    TS_ASSERT_EQUALS( outsptr1->getInstrument(), outsptr6->getInstrument() )
    TS_ASSERT_EQUALS( &(outsptr1->spectraMap()), &(outsptr6->spectraMap()) )
    TS_ASSERT_DIFFERS( outsptr1->getSample(), outsptr6->getSample() )
  }

  void testWithManagedWorkspace()
  {
    ConfigService::Instance().loadConfig("UseManagedWS.properties");
    LoadRaw3 loader4;
    loader4.initialize();
    loader4.setPropertyValue("Filename", inputFile);
    loader4.setPropertyValue("OutputWorkspace", "managedws2");
    TS_ASSERT_THROWS_NOTHING( loader4.execute() )
    TS_ASSERT( loader4.isExecuted() )

    // Get back workspace and check it really is a ManagedWorkspace2D
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("managedws2") );
    TS_ASSERT( dynamic_cast<ManagedWorkspace2D*>(output.get()) )
  }


  // test if parameters set in instrument definition file are loaded properly
  void testIfParameterFromIDFLoaded()
  {
    LoadRaw3 loader4;
    loader4.initialize();
    loader4.setPropertyValue("Filename", "../../../../Test/Data/TSC10076.raw");
    loader4.setPropertyValue("OutputWorkspace", "parameterIDF");
    TS_ASSERT_THROWS_NOTHING( loader4.execute() )
    TS_ASSERT( loader4.isExecuted() )

    // Get back workspace and check it really is a ManagedWorkspace2D
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieve("parameterIDF") );

    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);

    boost::shared_ptr<IInstrument> i = output2D->getInstrument();
    Mantid::Geometry::IDetector_sptr ptrDet = i->getDetector(60);
    TS_ASSERT_EQUALS( ptrDet->getID(), 60);

    Mantid::Geometry::ParameterMap& pmap = output2D->instrumentParameters();
    TS_ASSERT_EQUALS( pmap.size(), 140);
  }

  void testTwoTimeRegimes()
  {
    LoadRaw3 loader5;
    loader5.initialize();
    loader5.setPropertyValue("Filename", "../../../../Test/Data/IRS38633.raw");
    loader5.setPropertyValue("OutputWorkspace", "twoRegimes");
    loader5.setPropertyValue("SpectrumList", "2,3");
    TS_ASSERT_THROWS_NOTHING( loader5.execute() )
    TS_ASSERT( loader5.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT( output = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("twoRegimes")) )
    // Shift should be 3300 - check a couple of values
    TS_ASSERT_EQUALS( output->readX(0).front()+3300, output->readX(1).front() )
    TS_ASSERT_EQUALS( output->readX(0).back()+3300, output->readX(1).back() )

    AnalysisDataService::Instance().remove("twoRegimes");
  }


private:
  LoadRaw3 loader,loader2,loader3;
  std::string inputFile;
  std::string outputSpace;
};

#endif /*LoadRaw3TEST_H_*/
