#ifndef NORMALISETOMONITORTEST_H_
#define NORMALISETOMONITORTEST_H_

#include <cxxtest/TestSuite.h>
#include "WorkspaceCreationHelper.hh"

#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidAPI/SpectraDetectorMap.h"

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
    // Now need to set up a minimal instrument and spectra-detector map
    int forSpecDetMap[3] = {0,1,2};
    input->getAxis(1)->spectraNo(0) = 0;
    input->getAxis(1)->spectraNo(1) = 1;
    input->getAxis(1)->spectraNo(2) = 2;
    Mantid::Geometry::Detector *mon = new Mantid::Geometry::Detector("monitor",NULL);
    mon->setID(0);
    boost::shared_ptr<Instrument> instr = boost::dynamic_pointer_cast<Instrument>(input->getInstrument());
    instr->add(mon);
    instr->markAsMonitor(mon);
    Mantid::Geometry::Detector *det = new Mantid::Geometry::Detector("NOTmonitor",NULL);
    det->setID(1);
    instr->add(det);
    instr->markAsDetector(det);
    input->mutableSpectraMap().populate(forSpecDetMap, forSpecDetMap, 3);

    AnalysisDataService::Instance().add("normMon",input);
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
    // Check it fails if MonitorIndex is set to a non-monitor spectrum
    TS_ASSERT_THROWS_NOTHING( norm.setPropertyValue("MonitorSpectrum","1") )
    TS_ASSERT_THROWS_NOTHING( norm.execute() )
    TS_ASSERT( ! norm.isExecuted() )

    // Now it should succeed
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
    TS_ASSERT( !output->isDistribution() )

    // Check the non-monitor spectra
    for (int i = 1; i < output->getNumberHistograms(); ++i)
    {
      for (int j = 0; j < output->blocksize(); ++j)
      {
        TS_ASSERT_EQUALS( output->readX(i)[j], j+5 )
        TS_ASSERT_EQUALS( output->readY(i)[j], 0.04 )
        TS_ASSERT_DELTA( output->readE(i)[j], 0.0602, 0.0001 )
      }
    }

    // Now check the monitor one
    for (int k = 0; k < output->blocksize(); ++k)
    {
      TS_ASSERT_EQUALS( output->readX(0)[k], k+5 )
      TS_ASSERT_EQUALS( output->readY(0)[k], 0.2 )
      TS_ASSERT_DELTA( output->readE(0)[k], 0.0657, 0.0001 )
    }
  }

};

#endif /*NORMALISETOMONITORTEST_H_*/
