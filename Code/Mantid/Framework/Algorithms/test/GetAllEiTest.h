#ifndef GETALLEI_TEST_H_
#define GETALLEI_TEST_H_

#include <memory>
#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/GetAllEi.h"
#include "MantidKernel/TimeSeriesProperty.h"


using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
class GetAllEiTester : public GetAllEi
{
public:
     void find_chop_speed_and_delay(const API::MatrixWorkspace_sptr &inputWS,
       double &chop_speed,double &chop_delay){
       GetAllEi::find_chop_speed_and_delay(inputWS,chop_speed,chop_delay);
     }
     bool filterLogProvided()const{
       return m_useFilterLog;
     }

};

class GetAllEiTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetAllEiTest *createSuite() { return new GetAllEiTest(); }
  static void destroySuite( GetAllEiTest *suite ) { delete suite; }

  GetAllEiTest(){
  }

public:
  void testName(){
    TS_ASSERT_EQUALS( m_getAllEi.name(), "GetAllEi" );
  }

  void testVersion(){
    TS_ASSERT_EQUALS( m_getAllEi.version(), 1 );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( m_getAllEi.initialize() );
    TS_ASSERT( m_getAllEi.isInitialized() );
  }
  //
  void test_validators_work(){

     MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1, 11, 10);
     m_getAllEi.initialize();
     m_getAllEi.setProperty("Workspace",ws);
     m_getAllEi.setProperty("OutputWorkspace","monitor_peaks");
     TSM_ASSERT_THROWS("should throw runtime error on validation as no appropriate logs are defined",m_getAllEi.execute(),std::runtime_error);
     auto log_messages = m_getAllEi.validateInputs();
     TSM_ASSERT_EQUALS("Three logs should fail",log_messages.size(),2);
     // add invalid property type
     ws->mutableRun().addLogData(new Kernel::PropertyWithValue<double>("Chopper_Speed",10.));
     auto log_messages2 = m_getAllEi.validateInputs();
     TSM_ASSERT_EQUALS("Two logs should fail",log_messages2.size(),2);

     TSM_ASSERT_DIFFERS("should fail for different reason ",log_messages["ChopperSpeedLog"],log_messages2["ChopperSpeedLog"]);
     // add correct property type:
     ws->mutableRun().clearLogs();
     ws->mutableRun().addLogData(new Kernel::TimeSeriesProperty<double>("Chopper_Speed"));
     log_messages = m_getAllEi.validateInputs();
     TSM_ASSERT_EQUALS("One log should fail",log_messages.size(),1);
     TSM_ASSERT("Filter log is not provided ",!m_getAllEi.filterLogProvided());
     ws->mutableRun().addLogData(new Kernel::TimeSeriesProperty<double>("Chopper_Delay"));
     ws->mutableRun().addLogData(new Kernel::TimeSeriesProperty<double>("proton_charge"));
     log_messages = m_getAllEi.validateInputs();
     TSM_ASSERT_EQUALS("All logs are defined",log_messages.size(),0);
     TSM_ASSERT("Filter log is provided ",m_getAllEi.filterLogProvided());

     m_getAllEi.setProperty("MonitorSpectraID",2);
     log_messages = m_getAllEi.validateInputs();
     TSM_ASSERT_EQUALS("Workspace should not have spectra with ID=2",log_messages.size(),1);
  }
  //
  void test_get_chopper_speed(){

     MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1, 11, 10);
     std::unique_ptr<Kernel::TimeSeriesProperty<double> > chopSpeed(new Kernel::TimeSeriesProperty<double>("Chopper_Speed"));
     std::unique_ptr<Kernel::TimeSeriesProperty<double> > chopDelay(new Kernel::TimeSeriesProperty<double>("Chopper_Delay"));
     std::unique_ptr<Kernel::TimeSeriesProperty<double> > goodFram(new Kernel::TimeSeriesProperty<double>("proton_charge"));
     m_getAllEi.initialize();
     m_getAllEi.setProperty("Workspace",ws);
     m_getAllEi.setProperty("OutputWorkspace","monitor_peaks");


  }


private:
  GetAllEiTester m_getAllEi;

};

#endif
