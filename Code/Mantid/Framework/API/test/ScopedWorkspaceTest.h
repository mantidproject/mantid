#ifndef MANTID_API_SCOPEDWORKSPACETEST_H_
#define MANTID_API_SCOPEDWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <boost/algorithm/string/predicate.hpp>

using namespace Mantid::API;

/// MockWorkspace copied from AnalysisDataServiceTest so I have something to add to the ADS
class MockWorkspace : public Workspace
{
  virtual const std::string id() const { return "MockWorkspace"; }
  virtual const std::string toString() const { return ""; }
  virtual size_t getMemorySize() const { return 1; }
};
typedef boost::shared_ptr<MockWorkspace> MockWorkspace_sptr;

class ScopedWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ScopedWorkspaceTest *createSuite() { return new ScopedWorkspaceTest(); }
  static void destroySuite( ScopedWorkspaceTest *suite ) { delete suite; }

  ScopedWorkspaceTest() :
    m_ads( AnalysisDataService::Instance() )
  {
    m_ads.clear();
  }

  ~ScopedWorkspaceTest()
  {
    m_ads.clear();
  }

  void test_emptyConstructor()
  {
    ScopedWorkspace test;
    // Should have name created
    TS_ASSERT( ! test.name().empty() );
    // However, nothing should be added under that name yet
    TS_ASSERT( ! m_ads.doesExist( test.name() ) );
  }

  void test_name()
  {
    ScopedWorkspace test;

    const std::string prefix("__ScopedWorkspace_");

    TS_ASSERT( boost::starts_with(test.name(), prefix) );
    TS_ASSERT_EQUALS( test.name().size(), prefix.size() + 16 );
  }

  void test_removedWhenOutOfScope()
  {
    TS_ASSERT_EQUALS( m_ads.getObjectNamesInclHidden().size(), 0 );
     
    { // Simulated scope 
      MockWorkspace_sptr ws = MockWorkspace_sptr(new MockWorkspace); 
  
      ScopedWorkspace test;
      m_ads.add(test.name(), ws);

      TS_ASSERT( m_ads.doesExist( test.name() ) );
    }
    
    // Should be removed when goes out of scope
    TS_ASSERT_EQUALS( m_ads.getObjectNamesInclHidden().size(), 0 );
  }

  void test_removedWhenException()
  {
    TS_ASSERT_EQUALS( m_ads.getObjectNamesInclHidden().size(), 0 );
    
    try 
    {
      MockWorkspace_sptr ws = MockWorkspace_sptr(new MockWorkspace); 
  
      ScopedWorkspace test;
      m_ads.add(test.name(), ws);

      TS_ASSERT( m_ads.doesExist( test.name() ) );

      throw std::runtime_error("Exception");

      TS_FAIL("Shouldn't get there");
    }
    catch(...)
    {}

    TS_ASSERT_EQUALS( m_ads.getObjectNamesInclHidden().size(), 0 );
  }

  void test_workspaceGroups()
  {
    TS_ASSERT_EQUALS( m_ads.getObjectNamesInclHidden().size(), 0 );
    
    { // Simulated scope
      MockWorkspace_sptr ws1 = MockWorkspace_sptr(new MockWorkspace);
      MockWorkspace_sptr ws2 = MockWorkspace_sptr(new MockWorkspace);

      WorkspaceGroup_sptr wsGroup = WorkspaceGroup_sptr(new WorkspaceGroup);

      wsGroup->addWorkspace(ws1);
      wsGroup->addWorkspace(ws2);

      ScopedWorkspace testGroup;
      m_ads.add(testGroup.name(), wsGroup);

      TS_ASSERT_EQUALS( m_ads.getObjectNamesInclHidden().size(), 3 );
    }
  
    // Whole group should be removed
    TS_ASSERT_EQUALS( m_ads.getObjectNamesInclHidden().size(), 0 );
  }

private:
  AnalysisDataServiceImpl& m_ads;
};


#endif /* MANTID_API_SCOPEDWORKSPACETEST_H_ */
