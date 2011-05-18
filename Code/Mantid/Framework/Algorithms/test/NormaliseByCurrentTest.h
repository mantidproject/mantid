#ifndef NORMALISEBYCURRENTTEST_H_
#define NORMALISEBYCURRENTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/NormaliseByCurrent.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class NormaliseByCurrentTest : public CxxTest::TestSuite
{
public:
  NormaliseByCurrentTest()
  {
  }

  void testName()
  {
    TS_ASSERT_EQUALS( norm.name(), "NormaliseByCurrent" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( norm.version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( norm.category(), "General" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( norm.initialize() );
    TS_ASSERT( norm.isInitialized() );
  }

  void testNotInitialized()
  {
    if ( !norm.isInitialized() ) norm.initialize();

    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS( norm.execute(), std::runtime_error );
    TS_ASSERT( ! norm.isExecuted() );

  }

  MatrixWorkspace_const_sptr doTest(std::string wsNameIn, std::string wsNameOut, double expectedY, double expectedE)
  {
    NormaliseByCurrent norm1;
    if ( !norm1.isInitialized() ) norm1.initialize();

    TS_ASSERT_THROWS_NOTHING( norm1.setPropertyValue("InputWorkspace",wsNameIn) );
    TS_ASSERT_THROWS_NOTHING( norm1.setPropertyValue("OutputWorkspace",wsNameOut) );

    // Check it fails if charge hasn't been set.
    TS_ASSERT( ! norm1.execute() );
    TS_ASSERT( ! norm1.isExecuted() );
    // Now set the charge
    MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameIn));
    input->mutableRun().setProtonCharge(2.0);
    input->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    input->setYUnit("Counts");

    TS_ASSERT_THROWS_NOTHING( norm1.execute() );
    TS_ASSERT( norm1.isExecuted() );

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)) );

    for (size_t i=0; i < output->getNumberHistograms(); i++)
    {
      const MantidVec &  inX = input->readX(i);
      const MantidVec &  X = output->readX(i);
      const MantidVec &  Y = output->dataY(i);
      const MantidVec &  E = output->dataE(i);
      for (size_t j=0; j < Y.size(); j++)
      {
        TS_ASSERT_EQUALS( X[j], inX[j] );
        TS_ASSERT_EQUALS( Y[j], expectedY );
        TS_ASSERT_DELTA( E[j], expectedE, 1e-5 );
      }
    }

    TS_ASSERT_EQUALS( output->YUnit(), "Counts" );
    TS_ASSERT_EQUALS( output->YUnitLabel(), "Counts per microAmp.hour" );

    return output;

  }

  void testExec()
  {
    AnalysisDataService::Instance().add("normIn",WorkspaceCreationHelper::Create2DWorkspaceBinned(10,3,1));
    doTest("normIn", "normOut", 1.0, sqrt(2.0)/2.0);
    AnalysisDataService::Instance().remove("normIn");
    AnalysisDataService::Instance().remove("normOut");
  }

  void xtestExec_InPlace()
  {
    AnalysisDataService::Instance().add("normIn",WorkspaceCreationHelper::Create2DWorkspaceBinned(10,3,1));
    doTest("normIn", "normIn", 1.0, sqrt(2.0)/2.0);
    AnalysisDataService::Instance().remove("normIn");
  }

  void testExecEvent()
  {
    AnalysisDataService::Instance().add("normInEvent",WorkspaceCreationHelper::CreateEventWorkspace(10,3,100, 0.0, 1.0, 2));

    EventWorkspace_const_sptr outputEvent;
    outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(doTest("normInEvent", "normOutEvent", 1.0, sqrt(2.0)/2.0));
    // Output is an event workspace
    TS_ASSERT(outputEvent);

    AnalysisDataService::Instance().remove("normInEvent");
    AnalysisDataService::Instance().remove("normOutEvent");
  }

  void testExecEvent_InPlace()
  {
    AnalysisDataService::Instance().add("normInEvent",WorkspaceCreationHelper::CreateEventWorkspace(10,3,100, 0.0, 1.0, 2));

    EventWorkspace_const_sptr outputEvent;
    outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(doTest("normInEvent", "normInEvent", 1.0, sqrt(2.0)/2.0));
    // Output is an event workspace
    TS_ASSERT(outputEvent);

    AnalysisDataService::Instance().remove("normInEvent");
  }



  void testExecZero()
  {
    AnalysisDataService::Instance().add("normIn",WorkspaceCreationHelper::Create2DWorkspace123(3,10,1));

    NormaliseByCurrent norm1;
    norm1.initialize();

    TS_ASSERT_THROWS_NOTHING( norm1.setPropertyValue("InputWorkspace","normIn") );
    TS_ASSERT_THROWS_NOTHING( norm1.setPropertyValue("OutputWorkspace","normOut") );

    // Set the charge to zero
    MatrixWorkspace_sptr input = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("normIn"));
    input->mutableRun().setProtonCharge(0.0);
    input->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    input->setYUnit("Counts");

    TS_ASSERT_THROWS_NOTHING( norm1.execute() );
    TS_ASSERT( !norm1.isExecuted() );

    AnalysisDataService::Instance().remove("normIn");
    AnalysisDataService::Instance().remove("normOut");
  }

private:
  NormaliseByCurrent norm;
};

#endif /*NORMALISEBYCURRENTTEST_H_*/
