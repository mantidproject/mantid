#ifndef NORMALISETOMONITORTEST_H_
#define NORMALISETOMONITORTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;

class NormaliseToMonitorTest : public CxxTest::TestSuite
{
private:
  NormaliseToMonitor norm;

public:
  NormaliseToMonitorTest()
  {
    MatrixWorkspace_sptr input = WorkspaceCreationHelper::Create2DWorkspace123(10,3,1);
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
    Mantid::Geometry::Detector *mon = new Mantid::Geometry::Detector("monitor",NULL);
    mon->setID(0);
    boost::shared_ptr<Instrument> instr = boost::dynamic_pointer_cast<Instrument>(input->getBaseInstrument());
    instr->add(mon);
    instr->markAsMonitor(mon);
    Mantid::Geometry::Detector *det = new Mantid::Geometry::Detector("NOTmonitor",NULL);
    det->setID(1);
    instr->add(det);
    instr->markAsDetector(det);
    input->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, 3);

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
    monWS->mutableSpectraMap().populate(forSpecDetMap2, forSpecDetMap2, 1);

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

  void testCategory()
  {
    TS_ASSERT_EQUALS( norm.category(), "General" )
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
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("normMon2")) )

    // Check the non-monitor spectra
    for (int i = 1; i < output->getNumberHistograms(); ++i)
    {
      for (int j = 0; j < output->blocksize(); ++j)
      {
        TS_ASSERT_EQUALS( output->readX(i)[j], j )
        TS_ASSERT_DELTA( output->readY(i)[j], 2, 0.00001 )
        TS_ASSERT_DELTA( output->readE(i)[j], 3.05941, 0.00001 )
      }
    }

    // Now check the monitor one
    for (int k = 0; k < output->blocksize(); ++k)
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
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("normMon3")) )
    TS_ASSERT( ! output->isDistribution() )
    TS_ASSERT( output->YUnit().empty() )

    // Check the non-monitor spectra
    for (int i = 1; i < output->getNumberHistograms(); ++i)
    {
      for (int j = 0; j < output->blocksize(); ++j)
      {
        TS_ASSERT_EQUALS( output->readX(i)[j], j )
        TS_ASSERT_EQUALS( output->readY(i)[j], 0.04 )
        TS_ASSERT_DELTA( output->readE(i)[j], 0.0602, 0.0001 )
      }
    }

    // Now check the monitor one
    for (int k = 0; k < output->blocksize(); ++k)
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
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("normMon4")) )
    TS_ASSERT( ! output->isDistribution() )
    TS_ASSERT( output->YUnit().empty() )

    // Check the non-monitor spectra
    for (int i = 1; i < output->getNumberHistograms(); ++i)
    {
      for (int j = 0; j < output->blocksize(); ++j)
      {
        TS_ASSERT_EQUALS( output->readX(i)[j], j )
        TS_ASSERT_DELTA( output->readY(i)[j], 0.0323, 0.0001 )
        TS_ASSERT_DELTA( output->readE(i)[j], 0.0485, 0.0001 )
      }
    }

    // Now check the monitor one
    for (int k = 0; k < output->blocksize(); ++k)
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
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("MonitorSpectrum","0") )
    TS_ASSERT_THROWS_NOTHING( norm3.setPropertyValue("MonitorWorkspace","monWS") )
    TS_ASSERT_THROWS_NOTHING( norm3.execute() )
    TS_ASSERT( ! norm3.isExecuted() )
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
};

#endif /*NORMALISETOMONITORTEST_H_*/
