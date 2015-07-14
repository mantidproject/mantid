#ifndef CONVERTTOMATRIXWORKSPACETEST_H_
#define CONVERTTOMATRIXWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "MantidAlgorithms/ConvertToMatrixWorkspace.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;


class ConvertToMatrixWorkspaceTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
	Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    TS_ASSERT_EQUALS( cloner.name(), "ConvertToMatrixWorkspace" )
  }

  void testVersion()
  {
	Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    TS_ASSERT_EQUALS( cloner.version(), 1 )
  }

  void testInit()
  {
    Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    TS_ASSERT_THROWS_NOTHING( cloner.initialize() )
    TS_ASSERT( cloner.isInitialized() )
  }


  void testExec_2D_to_2D()
  {
	Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    if ( !cloner.isInitialized() ) cloner.initialize();
	//create 2D input workspace
	Mantid::API::MatrixWorkspace_sptr in = WorkspaceCreationHelper::Create2DWorkspace(5,10);
	//add instance to variable 'in' 
	Mantid::API::AnalysisDataService::Instance().addOrReplace("in", in);
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("InputWorkspace","in") )
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("OutputWorkspace","out") )
    TS_ASSERT( cloner.execute() )
	
	Mantid::API::MatrixWorkspace_sptr out;
	//retrieve OutputWorkspace produced by execute and set it to out
	TS_ASSERT_THROWS_NOTHING( out = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("out") );
	TS_ASSERT(out);
	if (!out) return;
	
	
    // Best way to test this is to use the CheckWorkspacesMatch algorithm
    Mantid::Algorithms::CheckWorkspacesMatch checker;
    checker.initialize();
    checker.setPropertyValue("Workspace1","in");
    checker.setPropertyValue("Workspace2","out");
    checker.execute();

    TS_ASSERT_EQUALS( checker.getPropertyValue("Result"), checker.successString() );
	Mantid::API::AnalysisDataService::Instance().remove("in");
  }

  void testExec_Event_to_2D()
  {
	Mantid::Algorithms::ConvertToMatrixWorkspace cloner;
    if ( !cloner.isInitialized() ) cloner.initialize();

	Mantid::DataObjects::EventWorkspace_sptr in = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 10);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("in", in);
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("InputWorkspace", "in") )
    TS_ASSERT_THROWS_NOTHING( cloner.setPropertyValue("OutputWorkspace","out") )
    TS_ASSERT( cloner.execute() )

    Mantid::API::MatrixWorkspace_sptr out;
    TS_ASSERT_THROWS_NOTHING(
        out = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("out") );
    TS_ASSERT(out);
    if (!out) return;

    TS_ASSERT_EQUALS( in->getNumberHistograms(), out->getNumberHistograms());
    TS_ASSERT_EQUALS( in->getInstrument()->getName(), out->getInstrument()->getName());
    TS_ASSERT_EQUALS( in->getInstrument()->isParametrized(), out->getInstrument()->isParametrized());
    for (size_t i=0; i < out->getNumberHistograms(); i++)
    {
      const Mantid::API::ISpectrum * inSpec = in->getSpectrum(i);
      const Mantid::API:: ISpectrum * outSpec = out->getSpectrum(i);
      TS_ASSERT_EQUALS( inSpec->getSpectrumNo(), outSpec->getSpectrumNo());
      TS_ASSERT_EQUALS( *inSpec->getDetectorIDs().begin(), *outSpec->getDetectorIDs().begin());
      TS_ASSERT_EQUALS( in->readX(i), out->readX(i));
      TS_ASSERT_EQUALS( in->readY(i), out->readY(i));
      TS_ASSERT_EQUALS( in->readE(i), out->readE(i));
    }

    Mantid::API::AnalysisDataService::Instance().remove("in");
  }
};

#endif /*CONVERTTOMATRIXWORKSPACETEST_H_*/
