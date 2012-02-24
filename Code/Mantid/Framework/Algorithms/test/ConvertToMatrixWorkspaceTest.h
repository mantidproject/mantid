#ifndef CONVERTTOMATRIXWORKSPACETEST_H_
#define CONVERTTOMATRIXWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvertToMatrixWorkspace.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ISpectrum.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class ConvertToMatrixWorkspaceTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( cloner.name(), "ConvertToMatrixWorkspace" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( cloner.version(), 1 )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( cloner.initialize() )
    TS_ASSERT( cloner.isInitialized() )
  }


  void testExec_2D_to_2D()
  {
    if ( !cloner.isInitialized() ) cloner.initialize();

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace", "in");
    loader.execute();

    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("InputWorkspace","in") )
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("OutputWorkspace","out") )

    TS_ASSERT( cloner.execute() )

    // Best way to test this is to use the CheckWorkspacesMatch algorithm
    Mantid::Algorithms::CheckWorkspacesMatch checker;
    checker.initialize();
    checker.setPropertyValue("Workspace1","in");
    checker.setPropertyValue("Workspace2","out");
    checker.execute();

    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), checker.successString() )
  }

  void testExec_Event_to_2D()
  {
    if ( !cloner.isInitialized() ) cloner.initialize();

    EventWorkspace_sptr in = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 10);
    AnalysisDataService::Instance().addOrReplace("in", in);
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("InputWorkspace", "in") )
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("OutputWorkspace","out") )
    TS_ASSERT( cloner.execute() )

    MatrixWorkspace_sptr out;
    TS_ASSERT_THROWS_NOTHING(
        out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out") );
    TS_ASSERT(out);
    if (!out) return;

    TS_ASSERT_EQUALS( in->getNumberHistograms(), out->getNumberHistograms());
    TS_ASSERT_EQUALS( in->getInstrument()->getName(), out->getInstrument()->getName());
    TS_ASSERT_EQUALS( in->getInstrument()->isParametrized(), out->getInstrument()->isParametrized());
    for (size_t i=0; i < out->getNumberHistograms(); i++)
    {
      const ISpectrum * inSpec = in->getSpectrum(i);
      const ISpectrum * outSpec = out->getSpectrum(i);
      TS_ASSERT_EQUALS( inSpec->getSpectrumNo(), outSpec->getSpectrumNo());
      TS_ASSERT_EQUALS( *inSpec->getDetectorIDs().begin(), *outSpec->getDetectorIDs().begin());
      TS_ASSERT_EQUALS( in->readX(i), out->readX(i));
      TS_ASSERT_EQUALS( in->readY(i), out->readY(i));
      TS_ASSERT_EQUALS( in->readE(i), out->readE(i));
    }

    AnalysisDataService::Instance().remove("in");
  }

private:
  Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
};

#endif /*CONVERTTOMATRIXWORKSPACETEST_H_*/
