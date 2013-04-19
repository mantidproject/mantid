/*
 * SelectionNotificationServiceTest.h
 *
 *  Created on: Mar 24, 2013
 *      Author: Ruth Mikkelson
 */

#ifndef SELECTIONNOTIFICATIONSERVICETEST_H_
#define SELECTIONNOTIFICATIONSERVICETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <Poco/NObserver.h>

#include "MantidKernel/MultiThreaded.h"
#include "MantidAPI/SelectionNotificationService.h"

using namespace Mantid;
using namespace API;

class SelectionNotificationServiceTest: public CxxTest::TestSuite
{
public:

  std::vector<int> vector;
  Poco::Mutex m_vectorMutex;

  void setUp()
  {
    SelectionNotificationService::Instance().clear();
  }

  void test_add()
  {
    TS_ASSERT_EQUALS(SelectionNotificationService::Instance().size(), 0);

    boost::shared_ptr<std::vector<double>> one(new std::vector<double>());
    one->push_back(3.0);
    one->push_back(5.0);
    one->push_back(7.0);

    // Add and check that it is in there
        TS_ASSERT_THROWS_NOTHING( SelectionNotificationService::Instance().add("one", one); );

        TS_ASSERT_EQUALS( SelectionNotificationService::Instance().size(), 1);
        TS_ASSERT( SelectionNotificationService::Instance().doesExist("one"));
        TS_ASSERT(SelectionNotificationService::Instance().retrieve("one") == one);

        // Can't re-add the same name
        TS_ASSERT_THROWS_ANYTHING( SelectionNotificationService::Instance().add("one", one); );
        // Can't add blank name
        TS_ASSERT_THROWS_ANYTHING( SelectionNotificationService::Instance().add("", one); );
      }

      void test_remove()
      {
        boost::shared_ptr<std::vector<double>> one(new std::vector<double>());
        one->push_back(3.0);
        one->push_back(5.0);
        one->push_back(7.0);
        TS_ASSERT_THROWS_NOTHING( SelectionNotificationService::Instance().add("one", one); );
        TS_ASSERT_EQUALS(SelectionNotificationService::Instance().size(), 1);
        TS_ASSERT_THROWS_NOTHING( SelectionNotificationService::Instance().remove("one"); );
        TS_ASSERT_EQUALS( SelectionNotificationService::Instance().size(), 0);

      }

      void test_addOrReplace()
      {
        TS_ASSERT_EQUALS( SelectionNotificationService::Instance().size(), 0);
        boost::shared_ptr<std::vector<double>> one(new std::vector<double>());
        one->push_back(3.0);
        one->push_back(5.0);
        one->push_back(7.0);
        TS_ASSERT_THROWS_NOTHING( SelectionNotificationService::Instance().add("one", one); );
        TS_ASSERT_EQUALS( SelectionNotificationService::Instance().size(), 1);

        // Does it replace?
        boost::shared_ptr<std::vector<double>> two(new std::vector<double>());
        two->push_back(1.0);
        two->push_back(2.0);
        two->push_back(3.0);
        TS_ASSERT_THROWS_NOTHING( SelectionNotificationService::Instance().addOrReplace("one", two); );
        TS_ASSERT_EQUALS( SelectionNotificationService::Instance().size(), 1);
        TS_ASSERT( SelectionNotificationService::Instance().doesExist("one"));

        // Was the name replaced? One equals two, what funny math!!!
        TS_ASSERT( SelectionNotificationService::Instance().retrieve("one") == two);
        boost::shared_ptr<std::vector<double> > Res =SelectionNotificationService::Instance().retrieve("one");
        std::vector<double> T = *Res;

        TS_ASSERT_DELTA( T.at(1) , 2.0,.001);

      }

      // Handler for an observer, called each time an object is added
      void handleAddNotification(const Poco::AutoPtr<SelectionNotificationServiceImpl::AddNotification> & x )
      {
        m_vectorMutex.lock();
        vector.push_back(123);
        std::cout<<"name="<<x->object_name()<<", object="<<x->object()->at(0)<<std::endl;
        m_vectorMutex.unlock();
      }

      void test_threadSafety()
      {
        Poco::NObserver<SelectionNotificationServiceTest, SelectionNotificationServiceImpl::AddNotification> observer(*this, &SelectionNotificationServiceTest::handleAddNotification);
        SelectionNotificationService::Instance().notificationCenter.addObserver(observer);
        vector.clear();

        boost::shared_ptr<std::vector<double>> object1(new std::vector<double>());
        object1->push_back(82.0);
        SelectionNotificationService::Instance().add("object1", object1);

        int num = 50;

        PARALLEL_FOR_NO_WSP_CHECK()
        for (int i=0; i<num; i++)
        {
          boost::shared_ptr<std::vector<double> > object(new std::vector<double>());
          object->push_back(i+23.0);
          object->push_back(i+30.0);

          std::ostringstream mess; mess << "item" << i;

          // Adding/replacing some items
          SelectionNotificationService::Instance().addOrReplace( mess.str(), object);

          // And retrieving some at the same time
          boost::shared_ptr<std::vector<double>> retrieved = SelectionNotificationService::Instance().retrieve("object1");
          std::vector<double> T=*retrieved;
          TS_ASSERT_DELTA( T.at(0), 82.0,.001);

          // Also add then remove another object
          std::string otherName = "other_" + mess.str();
          boost::shared_ptr<std::vector<double>> other(new std::vector<double>());
          other->push_back(i+40);
          other->push_back(i+50);
          SelectionNotificationService::Instance().add( otherName, other);
          SelectionNotificationService::Instance().remove( otherName );
        }

        TS_ASSERT_EQUALS ( SelectionNotificationService::Instance().size(), size_t(num+1) );

        // Vector was append twice per loop
        TS_ASSERT_EQUALS ( vector.size(), size_t(num*2+1) );

        // Try a few random items, check that they are there
        TS_ASSERT_EQUALS( (*SelectionNotificationService::Instance().retrieve("item19")).at(0), 19+23);
        TS_ASSERT_EQUALS( (*SelectionNotificationService::Instance().retrieve("item25")).at(1), 30+25);
      }

    };


#endif /* SELECTIONNOTIFICATIONSERVICETEST_H_ */
