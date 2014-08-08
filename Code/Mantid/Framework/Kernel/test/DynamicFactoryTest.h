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
class IntFactory : public DynamicFactory<int>
{
};

// Helper class
class CaseSensitiveIntFactory : public DynamicFactory<int,CaseSensitiveStringComparator>
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
    TS_ASSERT_THROWS_NOTHING( int_ptr i = factory.create("TESTENTRY") );
    factory.unsubscribe("testEntry");
  }

  void testCreateCaseSensitive()
  {
    TS_ASSERT_THROWS( caseSensitiveFactory.create("testEntryCaseSensitive"), std::runtime_error )
    caseSensitiveFactory.subscribe<int>("testEntryCaseSensitive");
    TS_ASSERT_THROWS( int_ptr i = caseSensitiveFactory.create("testEntryCaseSENSITIVE") , std::runtime_error); //case error on a case sensitive dynamic factory
    TS_ASSERT_THROWS_NOTHING( int_ptr i2 = caseSensitiveFactory.create("testEntryCaseSensitive") );
    caseSensitiveFactory.unsubscribe("testEntryCaseSensitive");
  }


  void testCreateUnwrapped()
  {
    TS_ASSERT_THROWS( factory.createUnwrapped("testUnrappedEntry"), std::runtime_error )
    factory.subscribe<int>("testUnwrappedEntry");
    int *i = NULL;
    TS_ASSERT_THROWS_NOTHING( i = factory.createUnwrapped("testUnwrappedEntry") );
    delete i;

    int *j = NULL;
    TS_ASSERT_THROWS_NOTHING( j = factory.createUnwrapped("TESTUnwrappedEntry") );
    delete j;
    factory.unsubscribe("testUnwrappedEntry");
  }

  void testCreateUnwrappedCaseSensitive()
  {
    TS_ASSERT_THROWS( caseSensitiveFactory.create("testUnrappedEntryCaseSensitive"), std::runtime_error )
    caseSensitiveFactory.subscribe<int>("testUnrappedEntryCaseSensitive");
    int *i = NULL;
    TS_ASSERT_THROWS( i = caseSensitiveFactory.createUnwrapped("testUnrappedentrycaseSENSITIVE") , std::runtime_error); //case error on a case sensitive dynamic factory
    TS_ASSERT_THROWS_NOTHING( i = caseSensitiveFactory.createUnwrapped("testUnrappedEntryCaseSensitive") );
    delete i;
    caseSensitiveFactory.unsubscribe("testUnrappedEntryCaseSensitive");
  }

  void testSubscribeWithEmptyNameThrowsInvalidArgument()
  {
    TS_ASSERT_THROWS( factory.subscribe<int>(""), std::invalid_argument);
  }

  void testSubscribeWithReplaceEqualsErrorIfExistsThrowsRegisteringMatchingClass()
  {
    TS_ASSERT_THROWS_NOTHING( factory.subscribe("int",new Instantiator<int, int>));
    TS_ASSERT_THROWS( factory.subscribe("int",new Instantiator<int, int>,IntFactory::ErrorIfExists), std::runtime_error);
    factory.unsubscribe("int");
  }

  void testSubscribeWithReplaceEqualsOverwriteCurrentReplacesMatchingClass()
  {
    TS_ASSERT_THROWS_NOTHING( factory.subscribe("int",new Instantiator<int, int>));
    TS_ASSERT_THROWS_NOTHING( factory.subscribe("int",new Instantiator<int, int>,IntFactory::OverwriteCurrent));

    factory.unsubscribe("int");
  }

  void testSubscribeByDefaultDoesNotNotify()
  {
    m_updateNoticeReceived = false;
    TS_ASSERT_THROWS_NOTHING( factory.subscribe<int>("int") );
    TS_ASSERT_EQUALS( m_updateNoticeReceived, false )
    factory.unsubscribe("int");
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
    TS_ASSERT( factory.exists("TESTING") );
  }
	
  void testGetKeys()
  {
    std::string testKey = "testGetKeys";
    //check it is not already present
    TS_ASSERT_THROWS( factory.create(testKey), std::runtime_error )
    factory.subscribe<int>(testKey);

    std::vector<std::string> keys = factory.getKeys();
    
    TSM_ASSERT("Could not find the test key in the returned vector.", std::find(keys.begin(), keys.end(), testKey)!=keys.end()) //check the case is correct

    TS_ASSERT(!keys.empty());

    factory.unsubscribe(testKey);
  }

  void testGetKeysRetainsCase()
  {
    std::string testKey = "testGetKeysRetainsCase";
    //check it is not already present
    TS_ASSERT_THROWS( factory.create(testKey), std::runtime_error )
    factory.subscribe<int>(testKey);

    std::vector<std::string> keys = factory.getKeys();
    
    factory.unsubscribe(testKey);

  }

private:
  void handleFactoryUpdate(const Poco::AutoPtr<IntFactory::UpdateNotification> &)
  {
    m_updateNoticeReceived = true;
  }

  IntFactory factory;
  CaseSensitiveIntFactory caseSensitiveFactory;
  Poco::NObserver<DynamicFactoryTest, IntFactory::UpdateNotification> m_notificationObserver;
  bool m_updateNoticeReceived;
};

#endif /*DYNAMICFACTORYTEST_H_*/
