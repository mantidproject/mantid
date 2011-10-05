#ifndef MANTID_CUSTOMINTERFACES_LOANEDMEMENTO_H_
#define MANTID_CUSTOMINTERFACES_LOANEDMEMENTO_H_

#include "MantidKernel/System.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Smart pointer wraps mementos so that they can be borrowed, but not deleted 
    class WorkspaceMemento;
    class DLLExport LoanedMemento {
    public:
      LoanedMemento (WorkspaceMemento *memento);
      LoanedMemento(const LoanedMemento& other);
      LoanedMemento& operator=(LoanedMemento& other);
      WorkspaceMemento * operator -> () ;
      ~LoanedMemento () ;
    private:
      WorkspaceMemento* m_memento;
    };
  }
}

#endif