#ifndef CHECKWORKSPACESMATCHTEST_H_
#define CHECKWORKSPACESMATCHTEST_H_

#include <cxxtest/TestSuite.h>

#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class CheckWorkspacesMatchTest : public CxxTest::TestSuite
{
public:
  CheckWorkspacesMatchTest() : loq("LOQ48127"), ws1(WorkspaceCreationHelper::Create2DWorkspace123(2,2))
  {
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "../../../../Test/AutoTestData/LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace", loq);
    loader.execute();
  }
  
  void testName()
  {
    TS_ASSERT_EQUALS( checker.name(), "CheckWorkspacesMatch" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( checker.version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( checker.category(), "General" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( checker.initialize() );
    TS_ASSERT( checker.isInitialized() );
  }
  
  void testMatches()
  {
	 if ( !checker.isInitialized() ) checker.initialize();
    
    // A workspace had better match itself!
    TS_ASSERT_THROWS_NOTHING( checker.setPropertyValue("Workspace1",loq) );
    TS_ASSERT_THROWS_NOTHING( checker.setPropertyValue("Workspace2",loq) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Success!" );
  }
  
  void testEvent_matches()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    AnalysisDataService::Instance().addOrReplace("ews1", ews1);
    AnalysisDataService::Instance().addOrReplace("ews2", ews2);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1","ews1") );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2","ews2") );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Success!" );
    AnalysisDataService::Instance().remove( "ews1" );
    AnalysisDataService::Instance().remove( "ews2" );
  }

  void testEvent_different_type()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    AnalysisDataService::Instance().addOrReplace("ews2", ews2);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2","ews2") );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_DIFFERS( checker.getPropertyValue("Result"), "Success!" );
    AnalysisDataService::Instance().remove( "ews2" );
  }

  void testEvent_different_number_histograms()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(15,20,30);
    AnalysisDataService::Instance().addOrReplace("ews1", ews1);
    AnalysisDataService::Instance().addOrReplace("ews2", ews2);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1","ews1") );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2","ews2") );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_DIFFERS( checker.getPropertyValue("Result"), "Success!" );
    AnalysisDataService::Instance().remove( "ews1" );
    AnalysisDataService::Instance().remove( "ews2" );
  }

  void testEvent_differentEventLists()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30, 0.0, 1.0, 2);
    AnalysisDataService::Instance().addOrReplace("ews1", ews1);
    AnalysisDataService::Instance().addOrReplace("ews2", ews2);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1","ews1") );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2","ews2") );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_DIFFERS( checker.getPropertyValue("Result"), "Success!" );
    AnalysisDataService::Instance().remove( "ews1" );
    AnalysisDataService::Instance().remove( "ews2" );
  }

  void testEvent_differentBinBoundaries()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    EventWorkspace_sptr ews1 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30, 15.0, 10.0);
    EventWorkspace_sptr ews2 = WorkspaceCreationHelper::CreateEventWorkspace(10,20,30, 5.0, 10.0);
    AnalysisDataService::Instance().addOrReplace("ews1", ews1);
    AnalysisDataService::Instance().addOrReplace("ews2", ews2);
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1","ews1") );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2","ews2") );
    TS_ASSERT( checker.execute() );
    TS_ASSERT_DIFFERS( checker.getPropertyValue("Result"), "Success!" );
    AnalysisDataService::Instance().remove( "ews1" );
    AnalysisDataService::Instance().remove( "ews2" );
  }

  void testDifferentSize()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create1DWorkspaceFib(2);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Size mismatch" );
    
    Mantid::API::AnalysisDataService::Instance().remove( "ws2" );
  }
  
  void testHistNotHist()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2,true);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Histogram/point-like mismatch" );
  }

  void testDistNonDist()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->isDistribution(true);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Distribution flag mismatch" );
  }

  void testDifferentAxisType()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    Mantid::API::Axis* const newAxis = new Mantid::API::NumericAxis(2);
    ws2->replaceAxis(1,newAxis);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() )
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Axis type mismatch" );
  }

  void testDifferentAxisTitles()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->getAxis(0)->title() = "blah";
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Axis title mismatch" )    ;
  }

  void testDifferentAxisUnit()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Axis unit mismatch" );
  }

  void testDifferentAxisValues()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->getAxis(1)->setValue(1,99);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() )
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Axis values mismatch" ) ;
  }

  void testDifferentYUnit()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->setYUnit("blah");
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "YUnit mismatch" );
  }

  void testDifferentSpectraMap()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    Mantid::API::SpectraDetectorMap& map = ws2->mutableSpectraMap();
    int spec[2] = {1,2};
    int det[2] = {99,98};
    map.populate(&spec[0],&det[0],2);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "SpectraDetectorMap mismatch" );
  }

  void testDifferentInstruments()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->getBaseInstrument()->setName("different");
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Instrument name mismatch" );
  }

  void testDifferentParameterMaps()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->instrumentParameters().addBool(new Mantid::Geometry::Component, "myParam", true);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Instrument ParameterMap mismatch" );
  }

  void testDifferentMasking()
  {
    if ( !checker.isInitialized() ) checker.initialize();

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->maskBin(0,0);
    ws2->dataY(0)[0] = 2;
    ws2->dataE(0)[0] = 3;
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Masking mismatch" );
    
    Mantid::API::MatrixWorkspace_sptr ws3 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws3->maskBin(0,1);
    ws3->dataY(0)[1] = 2;
    ws3->dataE(0)[1] = 3;

    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws3) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Masking mismatch" );
  }
  
  void testDifferentSampleName()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    checker.setProperty("CheckSample",true);

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->mutableSample().setName("different");
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Sample name mismatch" );
  }

  void testDifferentProtonCharge()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    checker.setProperty("CheckSample",true);

    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->mutableRun().setProtonCharge(99.99);
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Proton charge mismatch" );
  }

  void testDifferentLogs()
  {
    if ( !checker.isInitialized() ) checker.initialize();
    checker.setProperty("CheckSample",true);
    
    Mantid::API::MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws2->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<int>("Prop1",99));
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws1) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws2) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Different numbers of logs" );
    
    Mantid::API::MatrixWorkspace_sptr ws3 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws3->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<int>("Prop2",99));
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws2) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws3) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Log name mismatch" );
    
    Mantid::API::MatrixWorkspace_sptr ws4 = WorkspaceCreationHelper::Create2DWorkspace123(2,2);
    ws4->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<int>("Prop1",100));
    
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace1",ws2) );
    TS_ASSERT_THROWS_NOTHING( checker.setProperty("Workspace2",ws4) );
    
    TS_ASSERT( checker.execute() );
    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), "Log value mismatch" );
    
  }

private:
  Mantid::Algorithms::CheckWorkspacesMatch checker;
  const Mantid::API::MatrixWorkspace_sptr ws1;
  const std::string loq;
};

#endif /*CHECKWORKSPACESMATCHTEST_H_*/
