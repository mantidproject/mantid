#ifndef NORMALISETOMONITORTEST_H_
#define NORMALISETOMONITORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Property.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::Geometry::Instrument;

class NormaliseToMonitorTestHelper : public NormaliseToMonitor
{
public:
    void initialize(){
        NormaliseToMonitor::initialize();
    }
    void checkProperties(MatrixWorkspace_sptr inputWorkspace){
        NormaliseToMonitor::checkProperties(inputWorkspace);
    }

};


class NormaliseToMonitorTest : public CxxTest::TestSuite
{
private:
  NormaliseToMonitorTestHelper norm;

public:

  static NormaliseToMonitorTest *createSuite() { return new NormaliseToMonitorTest(); }
  static void destroySuite(NormaliseToMonitorTest *suite) { delete suite; }

  NormaliseToMonitorTest()
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
    // Now need to set up a minimal instrument and spectra-detector map
    int forSpecDetMap[3] = {0,1,2};
    input->getAxis(1)->spectraNo(0) = 0;
    input->getAxis(1)->spectraNo(1) = 1;
    input->getAxis(1)->spectraNo(2) = 2;
    boost::shared_ptr<Instrument> instr(new Instrument);
    input->setInstrument(instr);
    Mantid::Geometry::Detector *mon = new Mantid::Geometry::Detector("monitor",0,NULL);
    instr->add(mon);
    instr->markAsMonitor(mon);
    Mantid::Geometry::Detector *det = new Mantid::Geometry::Detector("NOTmonitor",1,NULL);
    instr->add(det);
    instr->markAsDetector(det);
    input->replaceSpectraMap(new SpectraDetectorMap(forSpecDetMap, forSpecDetMap, 3));

    AnalysisDataService::Instance().add("normMon",input);

    // Create a single spectrum workspace to be the monitor one
    MatrixWorkspace_sptr monWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,20,0.1,0.5);
    monWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    // Now need to set up a minimal instrument and spectra-detector map
    int forSpecDetMap2[1] = {0};
    monWS->getAxis(1)->spectraNo(0) = 0;
    monWS->setInstrument(input->getInstrument());
    //Mantid::Geometry::Detector *mon2 = new Mantid::Geometry::Detector("monitor",NULL);
    //mon2->setID(0);
    //instr = boost::dynamic_pointer_cast<Instrument>(monWS->getInstrument());
    //instr->add(mon2);
    //instr->markAsMonitor(mon2);
    monWS->replaceSpectraMap(new SpectraDetectorMap(forSpecDetMap2, forSpecDetMap2, 1));

    AnalysisDataService::Instance().add("monWS",monWS);
  }

  void testName()
  {
    TS_ASSERT_EQUALS( norm.name(), "NormaliseToMonitor" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( norm.version(), 1 )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( norm.initialize() )
    TS_ASSERT( norm.isInitialized() )
  }

  void testExec()
  {
    if ( !norm.isInitialized() ) norm.initialize();

    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS( norm.execute(), std::runtime_error )
    TS_ASSERT( ! norm.isExecuted() )

    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("InputWorkspace","normMon") )
    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("OutputWorkspace","normMon2") )
    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("MonitorSpectrum","0") )
    TS_ASSERT_THROWS_NOTHING( norm.execute() )
    TS_ASSERT( norm.isExecuted() )

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("normMon2") )

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
  }



  void testNormaliseByIntegratedCount()
  {
    NormaliseToMonitor norm2;
    norm2.initialize();
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("InputWorkspace","normMon") )
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("OutputWorkspace","normMon3") )
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("MonitorSpectrum","0") )
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("IntegrationRangeMin","5") )
    TS_ASSERT_THROWS_NOTHING( norm2.setPropertyValue("IntegrationRangeMax","20") )
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
  }

  void testFailsOnSettingBothMethods()
  {
    NormaliseToMonitor norm3;
    norm3.initialize();
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("InputWorkspace","normMon") )
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("OutputWorkspace","normMon3") )
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("MonitorWorkspaceIndex","0") )
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("MonitorWorkspace","monWS") )
    TS_ASSERT_THROWS_NOTHING( norm3.execute() )
    TS_ASSERT(  norm3.isExecuted() )
  }

  void testSeparateWorkspaceWithRebin()
  {
    NormaliseToMonitor norm4;
    norm4.initialize();
    TS_ASSERT_THROWS_NOTHING( norm4.setPropertyValue("InputWorkspace","normMon") )
    TS_ASSERT_THROWS_NOTHING( norm4.setPropertyValue("OutputWorkspace","normMon4") )
    TS_ASSERT_THROWS_NOTHING( norm4.setPropertyValue("MonitorWorkspace","monWS") )
    TS_ASSERT_THROWS_NOTHING( norm4.execute() )
    TS_ASSERT( norm4.isExecuted() )
  }

  void testMonIDPropChangerEnabled()
  {
    NormaliseToMonitor norm5;
    norm5.initialize();

    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("InputWorkspace","normMon"));
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("OutputWorkspace","normMon5"));

    std::auto_ptr<MonIDPropChanger> pID = std::auto_ptr<MonIDPropChanger>(new MonIDPropChanger(&norm5,"InputWorkspace","MonitorSpectrum","MonitorWorkspace"));

    // property is enabled but the conditions have not changed;
    TS_ASSERT(pID->isEnabled());
    // workspace has monitors so the condition has changed
    TS_ASSERT(pID->isConditionChanged());

    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorWorkspace","monWS"));
    // monitor ws disables this property;
    TS_ASSERT(!pID->isEnabled());
    // but no changes to condition for disabled property
    TS_ASSERT(!pID->isConditionChanged());

    // no mon ws should enable it again
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorWorkspace",""));
    TS_ASSERT(pID->isEnabled());
    TS_ASSERT(!pID->isConditionChanged());

    // and MonitorSpectrum disable: 
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorSpectrum","1"));
    TS_ASSERT(!pID->isEnabled());
    TS_ASSERT(!pID->isConditionChanged());
    // and enable:
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorSpectrum","-1"));
    TS_ASSERT(pID->isEnabled());
    TS_ASSERT(!pID->isConditionChanged());
    // and disable: 
    TS_ASSERT_THROWS_NOTHING(norm5.setPropertyValue("MonitorSpectrum","10"));
    TS_ASSERT(!pID->isEnabled());
    TS_ASSERT(!pID->isConditionChanged());

  }
  void testIsConditionChanged(){
        NormaliseToMonitor norm6;
        norm6.initialize();
        TS_ASSERT_THROWS_NOTHING(norm6.setPropertyValue("InputWorkspace","normMon"));
        TS_ASSERT_THROWS_NOTHING(norm6.setPropertyValue("OutputWorkspace","normMon6"));
        std::auto_ptr<MonIDPropChanger> pID = std::auto_ptr<MonIDPropChanger>(new MonIDPropChanger(&norm6,"InputWorkspace","MonitorSpectrum","MonitorWorkspace"));
        // first time in a row the condition has changed as it shluld read the monitors from the workspace
        TS_ASSERT(pID->isConditionChanged());
        // and second time the monitons should be the same so no changes
        TS_ASSERT(!pID->isConditionChanged());

  }
   void testAlgoConditionChanged(){
        NormaliseToMonitor norm6;
        norm6.initialize();
        TS_ASSERT_THROWS_NOTHING(norm6.setPropertyValue("InputWorkspace","normMon"));
        TS_ASSERT_THROWS_NOTHING(norm6.setPropertyValue("OutputWorkspace","normMon6"));

        
        Property* monSpec = norm6.getProperty("MonitorID");
        // this function is usually called by GUI when senning input workspace. It should read monitors and report the condition changed
        TS_ASSERT(monSpec->isConditionChanged());
        // this funciton is called by gui when the above is true. It should not throw and change the validator
        IPropertySettings *pSett;
        TS_ASSERT_THROWS_NOTHING(pSett= monSpec->getSettings());
        TS_ASSERT_THROWS_NOTHING(pSett->applyChanges(monSpec));
        // it should return the list of allowed monitor ID-s
        std::set<std::string> monitors = monSpec->allowedValues();
        TS_ASSERT_EQUALS(1,monitors.size());
        TS_ASSERT_EQUALS("0",*(monitors.begin()));

   }



};

#endif /*NORMALISETOMONITORTEST_H_*/
