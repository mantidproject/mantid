#ifndef DYNAMICFACTORYTEST_H_
#define DYNAMICFACTORYTEST_H_

#include "MantidKernel/DynamicFactory.h"
#include <cxxtest/TestSuite.h>

#include <boost/shared_ptr.hpp>
#include <Poco/NObserver.h>

#include <vector>
#include <string>


using namespace Mantid::Kernel;

// Helper class
class AFactory : public DynamicFactory<int>
{
};

class DynamicFactoryTest : public CxxTest::TestSuite
{
  typedef boost::shared_ptr<int> int_ptr;
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DynamicFactoryTest *createSuite() { return new DynamicFactoryTest(); }
  static void destroySuite( DynamicFactoryTest *suite ) { delete suite; }

  DynamicFactoryTest() :
    CxxTest::TestSuite(), factory(), m_notificationObserver(*this, &DynamicFactoryTest::handleFactoryUpdate), m_updateNoticeReceived(false)
  {
    factory.notificationCenter.addObserver(m_notificationObserver);
  }

  ~DynamicFactoryTest()
  {
    factory.notificationCenter.removeObserver(m_notificationObserver);
  }

  void testCreate()
  {
    TS_ASSERT_THROWS( factory.create("testEntry"), std::runtime_error )
    factory.subscribe<int>("testEntry");
    TS_ASSERT_THROWS_NOTHING( int_ptr i = factory.create("testEntry") );
  }

  void testCreateUnwrapped()
  {
    TS_ASSERT_THROWS( factory.createUnwrapped("testUnrappedEntry"), std::runtime_error )
    factory.subscribe<int>("testUnwrappedEntry");
    int *i = NULL;
    TS_ASSERT_THROWS_NOTHING( i = factory.createUnwrapped("testEntry") );
    delete i;
  }

  void testSubscribeByDefaultDoesNotNotify()
  {
    m_updateNoticeReceived = false;
    TS_ASSERT_THROWS_NOTHING( factory.subscribe<int>("int") );
    TS_ASSERT_EQUALS( m_updateNoticeReceived, false )
    TS_ASSERT_THROWS_NOTHING( factory.subscribe("int2",new Instantiator<int, int>));
    TS_ASSERT_THROWS( factory.subscribe<int>("int"), std::runtime_error);
  }

  void testSubscribeNotifiesIfTheyAreSwitchedOn()
  {
    m_updateNoticeReceived = false;
    factory.enableNotifications();
    TS_ASSERT_THROWS_NOTHING( factory.subscribe<int>("intWithNotice") );
    TS_ASSERT_EQUALS( m_updateNoticeReceived, true )
    factory.disableNotifications();
    TS_ASSERT_THROWS_NOTHING( factory.unsubscribe("intWithNotice"));
    }

  void testUnsubscribeByDefaultDoesNotNotify()
  {
     TS_ASSERT_THROWS( factory.unsubscribe("tester"), std::runtime_error);
     factory.subscribe<int>("tester");
     m_updateNoticeReceived = false;
     TS_ASSERT_THROWS_NOTHING( factory.unsubscribe("tester"));
     TS_ASSERT_EQUALS( m_updateNoticeReceived, false )
  }

  void testUnsubscribeNotifiesIfTheyAreSwitchedOn()
  {
    TS_ASSERT_THROWS_NOTHING( factory.subscribe<int>("intWithNotice") );
    factory.enableNotifications();
    m_updateNoticeReceived = false;
    TS_ASSERT_THROWS_NOTHING( factory.unsubscribe("intWithNotice"));
    TS_ASSERT_EQUALS( m_updateNoticeReceived, true )

    factory.disableNotifications();
    m_updateNoticeReceived = false;
  }

  void testExists()
  {
    TS_ASSERT( ! factory.exists("testing") );
    factory.subscribe<int>("testing");
    TS_ASSERT( factory.exists("testing") );
  }
	
  void testGetKeys()
  {
    std::vector<std::string> keys = factory.getKeys();
    TS_ASSERT(!keys.empty());
  }

private:
  void handleFactoryUpdate(const Poco::AutoPtr<AFactory::UpdateNotification> &)
  {
    m_updateNoticeReceived = true;
  }

  AFactory factory;
  Poco::NObserver<DynamicFactoryTest, AFactory::UpdateNotification> m_notificationObserver;
  bool m_updateNoticeReceived;
};

#endif /*DYNAMICFACTORYTEST_H_*/
