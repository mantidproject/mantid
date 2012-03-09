#ifndef MANTID_KERNEL_DATASERVICETEST_H_
#define MANTID_KERNEL_DATASERVICETEST_H_

#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/DataService.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <Poco/NObserver.h>
#include <vector>

using namespace Mantid;
using namespace Mantid::Kernel;

/// A simple data service
class DumbDataServiceImpl : public DataService<int>
{
public:
  DumbDataServiceImpl():
    DataService<int>("DumbDataServiceImpl")
  {
  }
};

class DataServiceTest : public CxxTest::TestSuite
{
public:
  /// A data service storing an int
  DumbDataServiceImpl svc;

  std::vector<int> vector;

  void setUp()
  {
    svc.clear();
  }

  void test_add()
  {
    TS_ASSERT_EQUALS( svc.size(), 0);
    boost::shared_ptr<int> one(new int(1));

    // Add and check that it is in there
    TS_ASSERT_THROWS_NOTHING( svc.add("one", one); );
    TS_ASSERT_EQUALS( svc.size(), 1);
    TS_ASSERT( svc.doesExist("one"));
    TS_ASSERT( svc.retrieve("one") == one);

    // Can't re-add the same name
    TS_ASSERT_THROWS_ANYTHING( svc.add("one", one); );
    // Can't add blank name
    TS_ASSERT_THROWS_ANYTHING( svc.add("", one); );
  }

  void test_remove()
  {
    boost::shared_ptr<int> one(new int(1));
    TS_ASSERT_THROWS_NOTHING( svc.add("one", one); );
    TS_ASSERT_EQUALS( svc.size(), 1);
    TS_ASSERT_THROWS_NOTHING( svc.remove("one"); );
    TS_ASSERT_EQUALS( svc.size(), 0);
  }

  void test_addOrReplace()
  {
    TS_ASSERT_EQUALS( svc.size(), 0);
    boost::shared_ptr<int> one(new int(1));
    TS_ASSERT_THROWS_NOTHING( svc.add("one", one); );
    TS_ASSERT_EQUALS( svc.size(), 1);

    // Does it replace?
    boost::shared_ptr<int> two(new int(2));
    TS_ASSERT_THROWS_NOTHING( svc.addOrReplace("one", two); );
    TS_ASSERT_EQUALS( svc.size(), 1);
    TS_ASSERT( svc.doesExist("one"));

    // Was the name replaced? One equals two, what funny math!!!
    TS_ASSERT( svc.retrieve("one") == two);
    TS_ASSERT_EQUALS( *svc.retrieve("one"), 2);
  }

  // Handler for an observer, called each time an object is added
  void handleAddNotification(const Poco::AutoPtr<DumbDataServiceImpl::AddNotification> & )
  {
    // This operation is not thread safe!
    // So it will segfault if called in parallel.
    vector.push_back(123);
  }

  void test_threadSafety()
  {
    // This starts observing the notifications of "add"
    Poco::NObserver<DataServiceTest, DumbDataServiceImpl::AddNotification> observer(*this, &DataServiceTest::handleAddNotification);
    svc.notificationCenter.addObserver(observer);
    vector.clear();

    boost::shared_ptr<int> object1(new int(12345));
    svc.add("object1", object1);

    int num = 5000;
    CPUTimer tim;
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

    std::cout << tim << " to add " << num << " items" << std::endl;

    TS_ASSERT_EQUALS ( svc.size(), size_t(num+1) );
    // Vector was append twice per loop
    TS_ASSERT_EQUALS ( vector.size(), size_t(num*2+1) );

    // Try a few random items, check that they are there
    TS_ASSERT_EQUALS( *svc.retrieve("item19"), 19);
    TS_ASSERT_EQUALS( *svc.retrieve("item765"), 765);
    TS_ASSERT_EQUALS( *svc.retrieve("item2345"), 2345);
  }


};


#endif /* MANTID_KERNEL_DATASERVICETEST_H_ */
