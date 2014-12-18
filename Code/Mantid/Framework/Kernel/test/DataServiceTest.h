#ifndef MANTID_KERNEL_DATASERVICETEST_H_
#define MANTID_KERNEL_DATASERVICETEST_H_

#include "MantidKernel/DataService.h"
#include "MantidKernel/MultiThreaded.h"
#include <cxxtest/TestSuite.h>
#include <Poco/NObserver.h>
#include <boost/make_shared.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;

/// A simple data service
class FakeDataService : public DataService<int>
{
public:
  FakeDataService():
    DataService<int>("FakeDataService")
  {
  }
};

class DataServiceTest : public CxxTest::TestSuite
{
private:
  // A data service storing an int
  FakeDataService svc;

  int notificationFlag; // A flag to help with testing notifications
  std::vector<int> vector;
  Mutex m_vectorMutex;

public:
  static DataServiceTest *createSuite() { return new DataServiceTest(); }
  static void destroySuite(DataServiceTest *suite) { delete suite; }

  void setUp()
  {
    svc.clear();
    notificationFlag = 0;
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces","0");
  }

  // Handler for an observer, called each time an object is added
  void handleAddNotification(const Poco::AutoPtr<FakeDataService::AddNotification>& )
  {
    Poco::ScopedLock<Mutex> _lock(m_vectorMutex);
    vector.push_back(123);
    ++notificationFlag;
  }

  void test_add()
  {
    Poco::NObserver<DataServiceTest, FakeDataService::AddNotification> observer(*this, &DataServiceTest::handleAddNotification);
    svc.notificationCenter.addObserver(observer);

    TS_ASSERT_EQUALS( svc.size(), 0);
    auto one = boost::make_shared<int>(1);

    // Add and check that it is in there
    TS_ASSERT_THROWS_NOTHING( svc.add("one", one); );
    TS_ASSERT_EQUALS( svc.size(), 1);
    TS_ASSERT( svc.doesExist("one"));
    TS_ASSERT( svc.retrieve("one") == one);

    // Same object can go in with different name
    svc.add("anotherOne", one);
    TS_ASSERT_EQUALS( svc.retrieve("anotherOne"), one );

    // Can't re-add the same name
    TS_ASSERT_THROWS( svc.add("one", one), std::runtime_error );
    // Can't add blank name
    TS_ASSERT_THROWS( svc.add("", one), std::runtime_error );
    // Can't add empty pointer
    TS_ASSERT_THROWS( svc.add("Null", boost::shared_ptr<int>()), std::runtime_error );

    svc.add("__hidden", boost::make_shared<int>(99));
    TS_ASSERT_EQUALS( notificationFlag, 3 )
    svc.notificationCenter.removeObserver(observer);
  }

  void handlePreDeleteNotification(const Poco::AutoPtr<FakeDataService::PreDeleteNotification>& notification)
  {
    TS_ASSERT_EQUALS( notification->objectName(), "one" );
    TS_ASSERT_EQUALS( *notification->object(), 1 );
    ++notificationFlag;
  }

  void handlePostDeleteNotification(const Poco::AutoPtr<FakeDataService::PostDeleteNotification>& notification)
  {
    TS_ASSERT_EQUALS( notification->objectName(), "one" );
    ++notificationFlag;
  }

  void test_remove()
  {
    Poco::NObserver<DataServiceTest, FakeDataService::PreDeleteNotification> observer(*this, &DataServiceTest::handlePreDeleteNotification);
    svc.notificationCenter.addObserver(observer);
    Poco::NObserver<DataServiceTest, FakeDataService::PreDeleteNotification> postobserver(*this, &DataServiceTest::handlePreDeleteNotification);
    svc.notificationCenter.addObserver(postobserver);
    TS_ASSERT_THROWS_NOTHING( svc.add("one", boost::make_shared<int>(1)); );
    TS_ASSERT_EQUALS( svc.size(), 1);
    TS_ASSERT_THROWS_NOTHING( svc.remove("two") ); // Nothing happens if the workspace isn't there
    TS_ASSERT_THROWS_NOTHING( svc.remove("one") );
    TS_ASSERT_EQUALS( svc.size(), 0);
    TS_ASSERT_EQUALS( notificationFlag, 2 );
    svc.notificationCenter.removeObserver(observer);
    svc.notificationCenter.removeObserver(postobserver);
  }

  void test_addOrReplace()
  {
    TS_ASSERT_EQUALS( svc.size(), 0);
    TS_ASSERT_THROWS_NOTHING( svc.add("one", boost::make_shared<int>(1)); );
    TS_ASSERT_EQUALS( svc.size(), 1);

    // Does it replace?
    boost::shared_ptr<int> two(new int(2));
    TS_ASSERT_THROWS_NOTHING( svc.addOrReplace("one", two); );
    TS_ASSERT_EQUALS( svc.size(), 1);
    TS_ASSERT( svc.doesExist("one"));

    // Was the name replaced? One equals two, what funny math!!!
    TS_ASSERT( svc.retrieve("one") == two);
    TS_ASSERT_EQUALS( *svc.retrieve("one"), 2);

    // Can't add blank names
    TS_ASSERT_THROWS( svc.addOrReplace("", two), std::runtime_error );
    // Can't add empty pointer
    TS_ASSERT_THROWS( svc.addOrReplace("one", boost::shared_ptr<int>()), std::runtime_error );
  }

  void handleAfterReplaceNotification(const Poco::AutoPtr<FakeDataService::AfterReplaceNotification>& )
  {
    ++notificationFlag;
  }

  void handleRenameNotification(const Poco::AutoPtr<FakeDataService::RenameNotification>& )
  {
    ++notificationFlag;
  }

  void test_rename()
  {
    Poco::NObserver<DataServiceTest, FakeDataService::AfterReplaceNotification> observer(*this, &DataServiceTest::handleAfterReplaceNotification);
    svc.notificationCenter.addObserver(observer);
    Poco::NObserver<DataServiceTest, FakeDataService::RenameNotification> observer2(*this, &DataServiceTest::handleRenameNotification);
    svc.notificationCenter.addObserver(observer2);

    auto one = boost::make_shared<int>(1);
    auto two = boost::make_shared<int>(2);
    svc.add("One", one);
    svc.add("Two", two);
    TS_ASSERT_EQUALS( svc.size(), 2 );

    TSM_ASSERT_THROWS_NOTHING("Nothing should happen if the names match", svc.rename("One","One") );
    TSM_ASSERT_THROWS_NOTHING("Should be just a warning if object not there", svc.rename("NotThere","NewName") );

    svc.rename("one","anotherOne"); // Note: Rename is case-insensitive on the old name
    TS_ASSERT_EQUALS( svc.size(), 2 );
    TSM_ASSERT_THROWS("One should have been renamed to anotherOne", svc.retrieve("one"), Exception::NotFoundError );
    TSM_ASSERT_EQUALS("One should have been renamed to anotherOne", svc.retrieve("anotherOne"), one );

    TSM_ASSERT_EQUALS("The observers should have been called 2 times in total", notificationFlag, 2 );

    svc.rename("Two","anotherOne");
    TS_ASSERT_EQUALS( svc.size(), 1);
    TSM_ASSERT_THROWS("Two should have been renamed to anotherOne", svc.retrieve("two"), Exception::NotFoundError );
    TSM_ASSERT_EQUALS("Two should have been renamed to anotherOne", svc.retrieve("anotherOne"), two );
    TSM_ASSERT_EQUALS("The observers should have been called 4 times in total", notificationFlag, 4 );

    svc.notificationCenter.removeObserver(observer);
    svc.notificationCenter.removeObserver(observer2);

    TS_ASSERT_THROWS( svc.rename("anotherOne",""), std::runtime_error);
    TSM_ASSERT_THROWS_NOTHING( "'AnotherOne' should still be there", svc.retrieve("anotherOne") );

  }

  void handleClearNotification(const Poco::AutoPtr<FakeDataService::ClearNotification>& )
  {
    ++notificationFlag;
  }

  void test_clear()
  {
    svc.add("something", boost::make_shared<int>(10) );
    TSM_ASSERT_LESS_THAN("Size should be 1", 0, svc.size() );
    svc.clear();
    TSM_ASSERT_EQUALS("DataService should be empty", svc.size(), 0 );
    TSM_ASSERT("handleClearNotification should not have been called", !notificationFlag );
    Poco::NObserver<DataServiceTest, FakeDataService::ClearNotification> observer(*this, &DataServiceTest::handleClearNotification);
    svc.notificationCenter.addObserver(observer);
    svc.add("something", boost::make_shared<int>(10) );
    svc.clear();
    TSM_ASSERT_EQUALS("DataService should be empty", svc.size(), 0 );
    TSM_ASSERT("handleClearNotification should have been called", notificationFlag );
    svc.notificationCenter.removeObserver(observer);
  }

  void test_retrieve_and_doesExist()
  {
    auto one = boost::make_shared<int>(1);
    svc.add("one", one );

    TS_ASSERT_EQUALS( svc.retrieve("one"), one );
    TSM_ASSERT_EQUALS( "Retrieval should be case-insensitive", svc.retrieve("oNE"), one );
    TS_ASSERT_THROWS( svc.retrieve("NOTone"), Exception::NotFoundError );

    TS_ASSERT( svc.doesExist("one") );
    TSM_ASSERT( "doesExist should be case-insensitive", svc.doesExist("oNE") );
    TS_ASSERT( ! svc.doesExist("NOTone") );
  }

  void test_size()
  {
    TS_ASSERT_EQUALS( svc.size(), 0 );
    svc.add("something", boost::make_shared<int>(-1) );
    TS_ASSERT_EQUALS( svc.size(), 1 );
    svc.add("__hidden", boost::make_shared<int>(1) );
    TSM_ASSERT_EQUALS( "Hidden workspaces should not be counted", svc.size(), 1 );

    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces","1");
    TS_ASSERT_EQUALS( svc.size(), 2 );
  }

  void test_getObjectNames_and_getObjects()
  {
    auto one = boost::make_shared<int>(1);
    auto two = boost::make_shared<int>(2);
    auto three = boost::make_shared<int>(3);
    svc.add("One", one);
    svc.add("Two", two);
    svc.add("TwoAgain", two); // Same pointer again - should appear twice in getObjects()
    svc.add("__Three", three);

    auto names = svc.getObjectNames();
    auto objects = svc.getObjects();
    TSM_ASSERT_EQUALS("Hidden entries should not be returned", names.size(), 3 );
    TSM_ASSERT_EQUALS("Hidden entries should not be returned", objects.size(), 3 );
    TS_ASSERT_DIFFERS( names.find("One"), names.end() );
    TS_ASSERT_DIFFERS( names.find("Two"), names.end() );
    TS_ASSERT_DIFFERS( names.find("TwoAgain"), names.end() );
    TS_ASSERT_EQUALS( names.find("__Three"), names.end() );
    TSM_ASSERT_EQUALS( "Hidden entries should not be returned", names.find("__Three"), names.end() );
    TS_ASSERT_EQUALS( objects.at(0), one );
    TS_ASSERT_EQUALS( objects.at(1), two );
    TS_ASSERT_EQUALS( objects.at(2), two );

    auto allNamesSize = svc.getObjectNamesInclHidden().size();
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces","1");
    TS_ASSERT_EQUALS( allNamesSize, svc.getObjectNamesInclHidden().size() )
    names = svc.getObjectNames();
    objects = svc.getObjects();
    TS_ASSERT_EQUALS( names.size(), 4 );
    TS_ASSERT_EQUALS( objects.size(), 4 );
    TS_ASSERT_DIFFERS( names.find("__Three"), names.end() );
    TS_ASSERT_EQUALS( objects.at(3), three );
  }

  void test_threadSafety()
  {
    // This starts observing the notifications of "add"
    Poco::NObserver<DataServiceTest, FakeDataService::AddNotification> observer(*this, &DataServiceTest::handleAddNotification);
    svc.notificationCenter.addObserver(observer);
    vector.clear();

    svc.add("object1", boost::make_shared<int>(12345));

    int num = 5000;
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i<num; i++)
    {
      boost::shared_ptr<int> object(new int(i));
      std::ostringstream mess;  mess << "item" << i;

      // Adding/replacing some items
      svc.addOrReplace( mess.str(), object);

      // And retrieving some at the same time
      boost::shared_ptr<int> retrieved = svc.retrieve("object1");
      TS_ASSERT_EQUALS( *retrieved, 12345);

      // Also add then remove another object
      std::string otherName = "other_" + mess.str();
      boost::shared_ptr<int> other(new int(i));
      svc.add( otherName, other);
      svc.remove( otherName );
    }

    TS_ASSERT_EQUALS ( svc.size(), size_t(num+1) );
    // Vector was append twice per loop
    TS_ASSERT_EQUALS ( vector.size(), size_t(num*2+1) );

    // Try a few random items, check that they are there
    TS_ASSERT_EQUALS( *svc.retrieve("item19"), 19);
    TS_ASSERT_EQUALS( *svc.retrieve("item765"), 765);
    TS_ASSERT_EQUALS( *svc.retrieve("item2345"), 2345);
  }

  void test_prefixToHide()
  {
    TS_ASSERT_EQUALS( FakeDataService::prefixToHide(), "__" );
  }

  void test_isHiddenDataServiceObject()
  {
    TS_ASSERT( FakeDataService::isHiddenDataServiceObject("__hidden") );
    TS_ASSERT( FakeDataService::isHiddenDataServiceObject("__HIDDEN") );
    TS_ASSERT( ! FakeDataService::isHiddenDataServiceObject("NotHidden") );
    TS_ASSERT( ! FakeDataService::isHiddenDataServiceObject("_NotHidden") );
    TS_ASSERT( ! FakeDataService::isHiddenDataServiceObject("NotHidden__") );
    TS_ASSERT( ! FakeDataService::isHiddenDataServiceObject("Not__Hidden") );
  }

  void test_showingHiddenObjects()
  {
    TS_ASSERT( ! FakeDataService::showingHiddenObjects() );
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces","1");
    TS_ASSERT( FakeDataService::showingHiddenObjects() );
    // Check behaviour if it's set to some invalid values
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces","invalid");
    TS_ASSERT( ! FakeDataService::showingHiddenObjects() );
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces","-1");
    TS_ASSERT( ! FakeDataService::showingHiddenObjects() );
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces","2");
    TS_ASSERT( ! FakeDataService::showingHiddenObjects() );
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces","^~");
    TS_ASSERT( ! FakeDataService::showingHiddenObjects() );
  }
};


#endif /* MANTID_KERNEL_DATASERVICETEST_H_ */
