#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
      LoanedMemento::LoanedMemento (WorkspaceMemento *memento) : m_memento (memento) 
      {
        if(m_memento)
        {
          m_memento->lock();
        }
      }

      LoanedMemento::LoanedMemento(const LoanedMemento& other)
      {
        m_memento = other.m_memento;
        if(m_memento)
        {
          other.m_memento->unlock();
          m_memento->lock();
        }
      }

      LoanedMemento& LoanedMemento::operator=(LoanedMemento& other)
      {
        if(this != &other)
        {
          m_memento = other.m_memento;
          if(m_memento)
          {
            other.m_memento->unlock();
            m_memento->lock();
          }
        }
        return *this;
      }

      WorkspaceMemento * LoanedMemento::operator -> () 
      {
        return m_memento;
      }

      LoanedMemento::~LoanedMemento () 
      {
        if(m_memento)
        {
          this->m_memento->unlock();
        }
        //Explicitly do not delete memento.
      }
  }
}