#ifndef CUSTOM_INTERFACES_SINGLE_OWNER_LOCK_TEST_H_
#define CUSTOM_INTERFACES_SINGLE_OWNER_LOCK_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/SingleOwnerLock.h"
using namespace MantidQt::CustomInterfaces;

class SingleOwnerLockTest : public CxxTest::TestSuite
{

public:

//=====================================================================================
// Functional tests
//=====================================================================================
   void testLock()
   {
     SingleOwnerLock lock("One");
     TS_ASSERT_THROWS_NOTHING(lock.lock());
     TS_ASSERT(lock.locked());
   }

   void testUnlock()
   {
     SingleOwnerLock lock("One");

     TS_ASSERT_THROWS_NOTHING(lock.lock());
     TS_ASSERT(lock.locked());
     TSM_ASSERT("Should unlock", lock.unlock());
     TS_ASSERT(!lock.locked());
     TSM_ASSERT("Should already be unlocked", !lock.unlock());
     TS_ASSERT(!lock.locked());
   }
   
   //Test that many single owner locks can operatate independently
   void testCreateThenFreeMany()
   {
     SingleOwnerLock a("a");
     SingleOwnerLock b("b");
     SingleOwnerLock c("c");

     a.lock();
     b.lock();
     c.lock();

     TS_ASSERT(a.locked());
     TS_ASSERT(b.locked());
     TS_ASSERT(c.locked());
     TS_ASSERT(a.unlock());
     TS_ASSERT(b.unlock());
     TS_ASSERT(c.unlock());
     TS_ASSERT(!a.locked());
     TS_ASSERT(!b.locked());
     TS_ASSERT(!c.locked());
   }

   void testLockSameTwiceThrows()
   {
     SingleOwnerLock a("One");
     TS_ASSERT_THROWS_NOTHING(a.lock());
     
     //Now try to lock the same object (keyed by name)
     SingleOwnerLock b("One");
     TSM_ASSERT_THROWS("Was already locked. Should have thrown.", b.lock(), std::runtime_error);
   }

   void testFreeOnDestruction()
   {
     {
       SingleOwnerLock a("One");
       a.lock();
     } //Now out of scope.

     SingleOwnerLock b("One");
     TSM_ASSERT_THROWS_NOTHING("First lock should have 'let go' on destruction.", b.lock());
   }
};

#endif