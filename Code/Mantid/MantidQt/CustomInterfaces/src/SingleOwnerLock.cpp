#include "MantidQtCustomInterfaces/SingleOwnerLock.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    SingleOwnerLock::LockMap SingleOwnerLock::locks;

    SingleOwnerLock::SingleOwnerLock(std::string wsName) : m_wsName(wsName) {}

      /// Apply the lock.
      void SingleOwnerLock::lock()
      {
        //You could get a race condition in the following, but this code is not intended for parallel usage.
        if(locked())
        {
          throw std::runtime_error("This memento is already in use!");
        }
        SingleOwnerLock::locks[m_wsName] = true;
      }

      /**
      Remove the lock.
      @return true if locked when unlocked. Returns false if already unlocked.
      */
      bool SingleOwnerLock::unlock()
      {
        bool existingState = SingleOwnerLock::locks[m_wsName];
        SingleOwnerLock::locks[m_wsName] = false;
        return existingState;
      }

      /**
      Getter for the locked status.
      @return true if locked.
      */
      bool SingleOwnerLock::locked() const
      {
        LockMap::iterator it = SingleOwnerLock::locks.find(m_wsName);
        if(SingleOwnerLock::locks.end() != it)
        {
          return it->second;
        }
        return false;
      }

      /// Destructor
      SingleOwnerLock::~SingleOwnerLock()
      {
        //Remove the lock if not done so already.
        if(locked())
          unlock();
      }
  }
}