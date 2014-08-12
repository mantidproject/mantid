#ifndef NORMALISETOMONITORTEST_H_
#define NORMALISETOMONITORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::Geometry::Instrument;

class NormaliseToMonitorTest : public CxxTest::TestSuite
{
public:
  static NormaliseToMonitorTest *createSuite() { return new NormaliseToMonitorTest(); }
  static void destroySuite(NormaliseToMonitorTest *suite) { delete suite; }

  void setUpWorkspace()
  {
    MatrixWorkspace_sptr input = WorkspaceCreationHelper::Create2DWorkspace123(3,10,1);
    // Change the data in the monitor spectrum
    input->dataY(0).assign(10,10.0);
    // Need to change bins
    for (int i = 0; i < 11; ++i)
    {
      input->dataX(0)[i] = i;
      input->dataX(1)[i] = i;
      input->dataX(2)[i] = i;
    }

     input->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    // Now need to set up a minimal instrument
    input->getSpectrum(0)->setSpectrumNo(0);
    input->getSpectrum(1)->setSpectrumNo(1);
    input->getSpectrum(2)->setSpectrumNo(2);
    boost::shared_ptr<Instrument> instr(new Instrument);
    input->setInstrument(instr);
    Mantid::Geometry::Detector *mon = new Mantid::Geometry::Detector("monitor",0,NULL);
    instr->add(mon);
    instr->markAsMonitor(mon);
    Mantid::Geometry::Detector *det = new Mantid::Geometry::Detector("NOTmonitor",1,NULL);
    instr->add(det);
    instr->markAsDetector(det);

    AnalysisDataService::Instance().addOrReplace("normMon",input);

    // Create a single spectrum workspace to be the monitor one
    MatrixWorkspace_sptr monWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,20,0.1,0.5);
    monWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    // Now need to set up a minimal instrument and spectra-detector map
    input->getSpectrum(0)->setSpectrumNo(0);
    monWS->setInstrument(input->getInstrument());

    AnalysisDataService::Instance().addOrReplace("monWS",monWS);
  }

  void testName()
  {
    NormaliseToMonitor norm;
    TS_ASSERT_EQUALS( norm.name(), "NormaliseToMonitor" )
  }

  void testVersion()
  {
    NormaliseToMonitor norm;
    TS_ASSERT_EQUALS( norm.version(), 1 )
  }

  void testInit()
  {
    NormaliseToMonitor norm;
    TS_ASSERT_THROWS_NOTHING( norm.initialize() )
    TS_ASSERT( norm.isInitialized() )
  }

  void dotestExec(bool events, bool sameOutputWS)
  {
    setUpWorkspace();

    NormaliseToMonitor norm;
    if (events)
      FrameworkManager::Instance().exec("ConvertToEventWorkspace", 8,
          "InputWorkspace", "normMon",
          "GenerateZeros", "1",
          "GenerateMultipleEvents", "0",
          "OutputWorkspace", "normMon");

    if ( !norm.isInitialized() ) norm.initialize();
    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS( norm.execute(), std::runtime_error )
    TS_ASSERT( ! norm.isExecuted() )
    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("InputWorkspace","normMon") )
    std::string outputWS("normMon");
    if ( ! sameOutputWS ) outputWS.append("2");
    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("MonitorSpectrum","0") )
    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("NormFactorWS","NormFactor") )
    TS_ASSERT_THROWS_NOTHING( norm.execute() )
    TS_ASSERT( norm.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWS) )

    // Check the non-monitor spectra
    for (size_t i = 1; i < output->getNumberHistograms(); ++i)
    {
      for (size_t j = 0; j < output->blocksize(); ++j)
      {
        TS_ASSERT_EQUALS( output->readX(i)[j], j )
        TS_ASSERT_DELTA( output->readY(i)[j], 2, 0.00001 )
        TS_ASSERT_DELTA( output->readE(i)[j], 3.05941, 0.00001 )
      }
    }

    // Now check the monitor one
    for (size_t k = 0; k < output->blocksize(); ++k)
    {
      TS_ASSERT_EQUALS( output->readX(0)[k], k )
      TS_ASSERT_DELTA( output->readY(0)[k], 10, 0.00001 )
      TS_ASSERT_DELTA( output->readE(0)[k], 4.24264, 0.00001 )
    }

    if (events)
    {
      EventWorkspace_const_sptr eventOut = boost::dynamic_pointer_cast<const EventWorkspace>(output);
      TS_ASSERT(eventOut);
    }
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("NormFactor") )
    TS_ASSERT_EQUALS(output->getNumberHistograms(),1);
    AnalysisDataService::Instance().remove("NormFactor");
  }


  void testExec()
  {
    dotestExec(false,false);
  }

  void testExec_Events()
  {
    dotestExec(true,false);
  }

  void testExec_inplace()
  {
    dotestExec(false,true);
  }

  void testExec_Events_inplace()
  {
    dotestExec(true,true);
  }


  void testNormaliseByIntegratedCount()
  {
    setUpWorkspace();

    NormaliseToMonitor norm2;
    norm2.initialize();
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("InputWorkspace","normMon") )
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("OutputWorkspace","normMon3") )
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("MonitorSpectrum","0") )
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("IntegrationRangeMin","5") )
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("IntegrationRangeMax","20") )
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("NormFactorWS","normMon3") )
    TS_ASSERT_THROWS_NOTHING( norm2.execute() )
    TS_ASSERT( norm2.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("normMon3") )
    TS_ASSERT( ! output->isDistribution() )
    TS_ASSERT( output->YUnit().empty() )

    // Check the non-monitor spectra
    for (size_t i = 1; i < output->getNumberHistograms(); ++i)
    {
      for (size_t j = 0; j < output->blocksize(); ++j)
      {
        TS_ASSERT_EQUALS( output->readX(i)[j], j )
        TS_ASSERT_EQUALS( output->readY(i)[j], 0.04 )
        TS_ASSERT_DELTA( output->readE(i)[j], 0.0602, 0.0001 )
      }
    }

    // Now check the monitor one
    for (size_t k = 0; k < output->blocksize(); ++k)
    {
      TS_ASSERT_EQUALS( output->readX(0)[k], k )
      TS_ASSERT_EQUALS( output->readY(0)[k], 0.2 )
      TS_ASSERT_DELTA( output->readE(0)[k], 0.0657, 0.0001 )
    }
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("normMon3_normFactor") )
    TS_ASSERT_EQUALS(output->getNumberHistograms(),1);
    AnalysisDataService::Instance().remove("normMon3_normFactor");

  }

  void testNormaliseByIntegratedCountIncPartBins()
  {
    NormaliseToMonitor norm3;
    norm3.initialize();
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("InputWorkspace","normMon") )
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("OutputWorkspace","normMon4") )
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("MonitorSpectrum","0") )
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("IntegrationRangeMin","3.5") )
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("IntegrationRangeMax","9.7") )
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("IncludePartialBins","1") )
    TS_ASSERT_THROWS_NOTHING( norm3.execute() )
    TS_ASSERT( norm3.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("normMon4") )
    TS_ASSERT( ! output->isDistribution() )
    TS_ASSERT( output->YUnit().empty() )

    // Check the non-monitor spectra
    for (size_t i = 1; i < output->getNumberHistograms(); ++i)
    {
      for (size_t j = 0; j < output->blocksize(); ++j)
      {
        TS_ASSERT_EQUALS( output->readX(i)[j], j )
        TS_ASSERT_DELTA( output->readY(i)[j], 0.0323, 0.0001 )
        TS_ASSERT_DELTA( output->readE(i)[j], 0.0485, 0.0001 )
      }
    }

    // Now check the monitor one
    for (size_t k = 0; k < output->blocksize(); ++k)
    {
      TS_ASSERT_EQUALS( output->readX(0)[k], k )
      TS_ASSERT_DELTA( output->readY(0)[k], 0.1613, 0.0001 )
      TS_ASSERT_DELTA( output->readE(0)[k], 0.0518, 0.0001 )
    }
    AnalysisDataService::Instance().remove("normMon4");
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("NormWS") );

  }

  void testFailsOnSettingBothMethods()
  {
    NormaliseToMonitor norm3;
    norm3.initialize();
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("InputWorkspace","normMon") );
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("OutputWorkspace","normMon3") );
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("MonitorWorkspaceIndex","0") );
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("MonitorWorkspace","monWS") );
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("NormFactorWS","NormWS") );
    TS_ASSERT_THROWS_NOTHING( norm3.execute() )
    TS_ASSERT(  norm3.isExecuted() );

    TS_ASSERT( AnalysisDataService::Instance().doesExist("NormWS") );
    AnalysisDataService::Instance().remove("NormWS");

  }

  void testSeparateWorkspaceWithRebin()
  {
    NormaliseToMonitor norm4;
    norm4.initialize();
    TS_ASSERT_THROWS_NOTHING( norm4.setPropertyValue("InputWorkspace","normMon") )
    TS_ASSERT_THROWS_NOTHING( norm4.setPropertyValue("OutputWorkspace","normMon4") )
    TS_ASSERT_THROWS_NOTHING( norm4.setPropertyValue("MonitorWorkspace","monWS") )  
    TS_ASSERT_THROWS_NOTHING( norm4.setPropertyValue("NormFactorWS","NormWS") );


    TS_ASSERT_THROWS_NOTHING( norm4.execute() )
    TS_ASSERT( norm4.isExecuted() )

    TS_ASSERT( AnalysisDataService::Instance().doesExist("NormWS") );
    AnalysisDataService::Instance().remove("NormWS");

  }

  void testMonIDPropChangerEnabled()
  {
    NormaliseToMonitor norm5;
    norm5.initialize();

    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("InputWorkspace","normMon"));
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("OutputWorkspace","normMon5"));

    std::auto_ptr<MonIDPropChanger> pID = std::auto_ptr<MonIDPropChanger>(new MonIDPropChanger("InputWorkspace","MonitorSpectrum","MonitorWorkspace"));

    // property is enabled but the conditions have not changed;
    TS_ASSERT(pID->isEnabled(&norm5));
    // workspace has monitors so the condition has changed
    TS_ASSERT(pID->isConditionChanged(&norm5));

    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorWorkspace","monWS"));
    // monitor ws disables this property;
    TS_ASSERT(!pID->isEnabled(&norm5));
    // but no changes to condition for disabled property
    TS_ASSERT(!pID->isConditionChanged(&norm5));

    // no mon ws should enable it again
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorWorkspace",""));
    TS_ASSERT(pID->isEnabled(&norm5));
    TS_ASSERT(!pID->isConditionChanged(&norm5));

    // and MonitorSpectrum disable: 
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorSpectrum","1"));
    TS_ASSERT(!pID->isEnabled(&norm5));
    TS_ASSERT(!pID->isConditionChanged(&norm5));
    // and enable:
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorSpectrum","-1"));
    TS_ASSERT(pID->isEnabled(&norm5));
    TS_ASSERT(!pID->isConditionChanged(&norm5));
    // and disable: 
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorSpectrum","10"));
    TS_ASSERT(!pID->isEnabled(&norm5));
    TS_ASSERT(!pID->isConditionChanged(&norm5));

  }
  void testIsConditionChanged(){
        NormaliseToMonitor norm6;
        norm6.initialize();
        TS_ASSERT_THROWS_NOTHING(norm6.setPropertyValue("InputWorkspace","normMon"));
        TS_ASSERT_THROWS_NOTHING(norm6.setPropertyValue("OutputWorkspace","normMon6"));
        std::auto_ptr<MonIDPropChanger> pID = std::auto_ptr<MonIDPropChanger>(new MonIDPropChanger("InputWorkspace","MonitorSpectrum","MonitorWorkspace"));
        // first time in a row the condition has changed as it shluld read the monitors from the workspace
        TS_ASSERT(pID->isConditionChanged(&norm6));
        // and second time the monitons should be the same so no changes
        TS_ASSERT(!pID->isConditionChanged(&norm6));

  }
   void testAlgoConditionChanged(){
        NormaliseToMonitor norm6;
        norm6.initialize();
        TS_ASSERT_THROWS_NOTHING(norm6.setPropertyValue("InputWorkspace","normMon"));
        TS_ASSERT_THROWS_NOTHING(norm6.setPropertyValue("OutputWorkspace","normMon6"));

        
        Property* monSpec = norm6.getProperty("MonitorID");
        // this function is usually called by GUI when senning input workspace. It should read monitors and report the condition changed
        TS_ASSERT(monSpec->getSettings()->isConditionChanged(&norm6));
        // this funciton is called by gui when the above is true. It should not throw and change the validator
        IPropertySettings *pSett(NULL);
        TS_ASSERT_THROWS_NOTHING(pSett= monSpec->getSettings());
        TS_ASSERT_THROWS_NOTHING(pSett->applyChanges(&norm6, monSpec));
        // it should return the list of allowed monitor ID-s
        std::vector<std::string> monitors = monSpec->allowedValues();
        TS_ASSERT_EQUALS(1,monitors.size());
        TS_ASSERT_EQUALS("0",*(monitors.begin()));

        // now deal with ws without monitors
       // create ws without monitors. 
        MatrixWorkspace_sptr input = WorkspaceCreationHelper::Create2DWorkspace123(3,10,1);
        boost::shared_ptr<Instrument> instr(new Instrument);
        input->setInstrument(instr);
        AnalysisDataService::Instance().add("someWS",input);

        TS_ASSERT_THROWS_NOTHING(norm6.setPropertyValue("InputWorkspace","someWS"));
        // this function is usually called by GUI when setting an input workspace. It should read monitors and report the condition changed
        TS_ASSERT(monSpec->getSettings()->isConditionChanged(&norm6));
        // this funciton is called by gui when the above is true. It should not throw and change the validator  
        TS_ASSERT_THROWS_NOTHING(pSett= monSpec->getSettings());
        TS_ASSERT_THROWS_NOTHING(pSett->applyChanges(&norm6, monSpec));
        // it should return the list of allowed monitor ID-s
        monitors = monSpec->allowedValues();
        TS_ASSERT(monitors.empty());



   }
  

};

#endif /*NORMALISETOMONITORTEST_H_*/
