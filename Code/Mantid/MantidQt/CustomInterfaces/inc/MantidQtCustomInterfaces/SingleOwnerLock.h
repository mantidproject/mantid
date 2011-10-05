#ifndef MANTID_CUSTOMINTERFACES_SINGLE_OWNER_LOCK_H_
#define MANTID_CUSTOMINTERFACES_SINGLE_OWNER_LOCK_H_

#include "MantidQtCustomInterfaces/WorkspaceMementoLock.h"
#include <map>
#include <string>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /**
    Concrete locking functor implementing single-owner-usage for locking.
    */
    class DLLExport SingleOwnerLock : public WorkspaceMementoLock
    {
    private:
      typedef std::map<std::string, bool> LockMap;
      static LockMap locks;
      std::string m_wsName;
    public:
      SingleOwnerLock(std::string wsName);
      virtual void lock();
      virtual bool unlock();
      virtual bool locked() const;
      virtual ~SingleOwnerLock();
    };
  }
}

#endif