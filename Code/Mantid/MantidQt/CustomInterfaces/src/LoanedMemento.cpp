#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
     
      /**
      Constructor. Locks provided memento.
      @param memento : WorkspaceMemento to wrap.
      @throws runtime_error if memento is null.
      */
      LoanedMemento::LoanedMemento (WorkspaceMemento *memento) : m_memento (memento) 
      {
        if(m_memento)
        {
          m_memento->lock();
        }
      }

      /**
      Copy constructor.
      */
      LoanedMemento::LoanedMemento(const LoanedMemento& other)
      {
        m_memento = other.m_memento;
      }

      /**
      Assignment operator
      @param other : assignment origin
      @return ref to assigned object
      */
      LoanedMemento& LoanedMemento::operator=(LoanedMemento& other)
      {
        if(this != &other)
        {
          if(m_memento != NULL)
          {
            this->m_memento->unlock();
          }
          this->m_memento = NULL;
          this->m_memento = other.m_memento;
        }
        return *this;
      }

      /**
      Recursive nature of pointer access ensures that WorkspaceMemento API can be used directly through LoanedMemento.
      @return ptr to held WorkspaceMemento.
      */
      WorkspaceMemento * LoanedMemento::operator -> () 
      {
        return m_memento;
      }

      /// Destructor
      LoanedMemento::~LoanedMemento () 
      {
        if(m_memento != NULL)
        {
          m_memento->unlock();
        }
      }

  }
}