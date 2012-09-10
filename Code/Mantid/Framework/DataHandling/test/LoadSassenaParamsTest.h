#ifndef MANTID_DATAHANDLING_LOADSASSENAPARAMSTEST_H_
#define MANTID_DATAHANDLING_LOADSASSENAPARAMSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadSassenaParams.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid;

class LOADSASSENAPARAMSTEST : public CxxTest::TestSuite
{
public:
  static LOADSASSENAPARAMSTEST *createSuite() { return new LOADSASSENAPARAMSTEST(); }
  static void destroySuite( LOADSASSENAPARAMSTEST *suite ) { delete suite; }

  LOADSASSENAPARAMSTEST()
  {
	  m_inputFile = "inputSassena_1.4.1.xml";
  }

  void test_init()
  {
    TS_ASSERT_THROWS_NOTHING( m_alg.initialize() )
    TS_ASSERT( m_alg.isInitialized() )
  }

  void test_optionalWorkspace()
  {
    if( !m_alg.isInitialized() ) m_alg.initialize();
    API::WorkspaceGroup_sptr gws = m_alg.getProperty("Workspace");
    TS_ASSERT(!gws); //Check NULL pointer
    gws = new API::WorkspaceGroup;
    API::AnalysisDataService::Instance().add( "GWS", gws);
    m_alg.setPropertyValue( "WorkSpace", "GWS" );
    API::WorkspaceGroup_sptr gws2 = m_alg.getProperty("WorkSpace");
    TS_ASSERT(gws==gws2);
  }

  void test_exec()
  {
    std::string result;
    if( !m_alg.isInitialized() ) m_alg.initialize();
    m_alg.setPropertyValue( "Filename", m_inputFile );
    TS_ASSERT_THROWS_NOTHING( m_alg.execute() );
    TS_ASSERT( m_alg.isExecuted() );

  } // end of testExec

private:
  std::string m_inputFile;
  Mantid::DataHandling::LoadSassenaParams m_alg;

}; // end of class LOADSASSENAPARAMSTEST

#endif // MANTID_DATAHANDLING_LOADSASSENAPARAMSTEST_H_
