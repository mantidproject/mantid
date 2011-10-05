#ifndef MANTID_CUSTOMINTERFACES_WORKSPACE_MEMENTO_LOCK_H_
#define MANTID_CUSTOMINTERFACES_WORKSPACE_MEMENTO_LOCK_H_

#include "MantidKernel/System.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /**
    Abstract Functor for locking usage. 
    */
    class DLLExport WorkspaceMementoLock
    {
    public:
      virtual void lock() = 0;
      virtual bool unlock() = 0;
      virtual bool locked() const = 0;
      virtual ~WorkspaceMementoLock(){}
    };
  }
}

#endif