#ifndef GETALLEI_TEST_H_
#define GETALLEI_TEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidAlgorithms/GetAllEi.h>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

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
  void testName()
  {
    TS_ASSERT_EQUALS( m_getAllEi.name(), "GetAllEi" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( m_getAllEi.version(), 1 );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( m_getAllEi.initialize() );
    TS_ASSERT( m_getAllEi.isInitialized() );
  }
  void testFail()
  {
     MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1, 11, 10);
     m_getAllEi.initialize();
     m_getAllEi.setProperty("Workspace",ws);
     m_getAllEi.setProperty("OutputWorkspace","monitor_peaks");
     TS_ASSERT_THROWS(m_getAllEi.execute(),std::runtime_error);

  }


private:
  GetAllEi m_getAllEi;

};

#endif
