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
      GetAllEi::findChopSpeedAndDelay(inputWS,chop_speed,chop_delay);
  }
  void findGuessOpeningTimes(const std::pair<double,double> &TOF_range,
    double ChopDelay,double Period,std::vector<double > & guess_opening_times){
      GetAllEi::findGuessOpeningTimes(TOF_range,ChopDelay,Period,guess_opening_times);
  }
  bool filterLogProvided()const{
    return m_useFilterLog;
  }
  double getAvrgLogValue(const API::MatrixWorkspace_sptr &inputWS, const std::string &propertyName){
    std::vector<Kernel::SplittingInterval> splitter;
    return GetAllEi::getAvrgLogValue(inputWS, propertyName,splitter);
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
    m_getAllEi.initialize();
    m_getAllEi.setProperty("Workspace",ws);
    m_getAllEi.setProperty("OutputWorkspace","monitor_peaks");
    m_getAllEi.setProperty("MonitorSpectraID",0);

    for(int i=0;i<10;i++){
      chopSpeed->addValue( Kernel::DateAndTime(10000+10*i, 0), 1.);
    }
    for(int i=0;i<10;i++){
      chopSpeed->addValue( Kernel::DateAndTime(100+10*i, 0), 10.);
    }
    for(int i=0;i<10;i++){
      chopSpeed->addValue( Kernel::DateAndTime(10*i, 0), 100.);
    }
    ws->mutableRun().addLogData(chopSpeed.release());

    // Test sort log by run time.
    TSM_ASSERT_THROWS("Attempt to get log without start/stop time set should fail",
      m_getAllEi.getAvrgLogValue(ws,"ChopperSpeedLog"),std::runtime_error);

    ws->mutableRun().setStartAndEndTime(Kernel::DateAndTime(90,0),Kernel::DateAndTime(10000,0));
    double val = m_getAllEi.getAvrgLogValue(ws,"ChopperSpeedLog");
    TS_ASSERT_DELTA(val,(10*10+100.)/11.,1.e-6);

    ws->mutableRun().setStartAndEndTime(Kernel::DateAndTime(100,0),Kernel::DateAndTime(10000,0));
    val = m_getAllEi.getAvrgLogValue(ws,"ChopperSpeedLog");
    TS_ASSERT_DELTA(val,10.,1.e-6);

    // Test sort log by log.
    std::unique_ptr<Kernel::TimeSeriesProperty<double> > chopDelay(new Kernel::TimeSeriesProperty<double>("Chopper_Delay"));
    std::unique_ptr<Kernel::TimeSeriesProperty<double> > goodFram(new Kernel::TimeSeriesProperty<double>("proton_charge"));

    for(int i=0;i<10;i++){
      auto time = Kernel::DateAndTime(200+10*i, 0);
      chopDelay->addValue(time , 10.);
      if (i<2){
        goodFram->addValue(time,1);
      }else{
        goodFram->addValue(time,0);
      }

    }
    for(int i=0;i<10;i++){
      auto time =  Kernel::DateAndTime(100+10*i, 0);
      chopDelay->addValue(time , 0.1);
      goodFram->addValue(time,1);
    }
    for(int i=0;i<10;i++){
      auto time =  Kernel::DateAndTime(10*i, 0);
      chopDelay->addValue(time , 1.);
      goodFram->addValue(time,0);
    }
    ws->mutableRun().addLogData(chopDelay.release());
    ws->mutableRun().addLogData(goodFram.release());
    // Run validate as this will set up property, which indicates filter log presence
    auto errors  = m_getAllEi.validateInputs();
    TSM_ASSERT_EQUALS("All logs are defined now",errors.size(),0);

    double chop_speed,chop_delay;
    m_getAllEi.find_chop_speed_and_delay(ws,chop_speed,chop_delay);
    TSM_ASSERT_DELTA("Chopper delay should have special speed ",(10*0.1+20)/12., chop_delay,1.e-6);

    goodFram.reset(new Kernel::TimeSeriesProperty<double>("proton_charge"));
    for(int i=0;i<10;i++){
      auto time =  Kernel::DateAndTime(100+10*i, 0);
      goodFram->addValue(time,1);
    }

    ws->mutableRun().addProperty(goodFram.release(),true);
    m_getAllEi.find_chop_speed_and_delay(ws,chop_speed,chop_delay);
    TSM_ASSERT_DELTA("Chopper delay should have special speed",0.1, chop_delay,1.e-6);

  }
  void test_guess_opening_times(){

    std::pair<double,double> TOF_range(5,100);
    double t0(6),Period(10);
    std::vector<double> guess_tof;
    m_getAllEi.findGuessOpeningTimes(TOF_range,t0,Period,guess_tof);
    TSM_ASSERT_EQUALS("should have 10 periods within the specified interval",guess_tof.size(),10);

    guess_tof.resize(0);
    t0 = TOF_range.first;
    m_getAllEi.findGuessOpeningTimes(TOF_range,t0,Period,guess_tof);
    TSM_ASSERT_EQUALS("Still should be 10 periods within the specified interval",guess_tof.size(),10);

    t0 = TOF_range.second;
    TSM_ASSERT_THROWS("Should throw out of range",m_getAllEi.findGuessOpeningTimes(TOF_range,t0,Period,guess_tof),std::runtime_error);

    t0 = 1;
    guess_tof.resize(0);
    m_getAllEi.findGuessOpeningTimes(TOF_range,t0,Period,guess_tof);
    TSM_ASSERT_EQUALS(" should be 9 periods within the specified interval",guess_tof.size(),9);

    guess_tof.resize(0);
    t0 = 21;
    TOF_range.first = 20;
    m_getAllEi.findGuessOpeningTimes(TOF_range,t0,Period,guess_tof);
    TSM_ASSERT_EQUALS(" should be 8 periods within the specified interval",guess_tof.size(),8);
 
  }


private:
  GetAllEiTester m_getAllEi;

};

#endif
