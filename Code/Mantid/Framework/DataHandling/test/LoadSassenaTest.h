#ifndef MANTID_DATAHANDLING_LOADSASSENATEST_H_
#define MANTID_DATAHANDLING_LOADSASSENATEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/LoadSassena.h"

class LoadSassenaTest : public CxxTest::TestSuite
{
public:
  static LoadSassenaTest *createSuite() { return new LoadSassenaTest(); }
  static void destroySuite( LoadSassenaTest *suite ) { delete suite; }

  LoadSassenaTest()
  {
    m_inputFile = "outputSassena_1.4.1.h5";
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( m_alg.initialize() )
    TS_ASSERT( m_alg.isInitialized() )
  }

  void testFileCheck()
  {
    if( !m_alg.isInitialized() ) m_alg.initialize();
    m_alg.setPropertyValue( "Filename", m_inputFile );
    TS_ASSERT_EQUALS(m_alg.fileCheck(m_alg.getPropertyValue("Filename")), 99);
  }

  void testExec()
  {
    std::string result;
    if( !m_alg.isInitialized() ) m_alg.initialize();

    m_alg.setPropertyValue( "Filename", m_inputFile );

    const std::string outSpace = "outGWS";
    m_alg.setPropertyValue( "OutputWorkSpace", outSpace );
    TS_ASSERT_THROWS_NOTHING( result = m_alg.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( result == outSpace );

    TS_ASSERT_THROWS_NOTHING( m_alg.execute() );
    TS_ASSERT( m_alg.isExecuted() );

  } // end of testExec

private:
  std::string m_inputFile;
  Mantid::DataHandling::LoadSassena m_alg;

}; // end of class LoadSassenaTest

#endif // MANTID_DATAHANDLING_LOADSASSENATEST_H_
