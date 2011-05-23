#include <vector>
#include <iterator>

#include "MantidAPI/LocatedDataRef.h"

namespace Mantid 
{
  namespace API
  {
    /**
    Null constructor
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container>::workspace_iterator() :
      m_workspace(0),m_CPoint(),m_loopCount(1),m_loopOrientation(1),
      m_index(0),m_wsSize(0),m_blocksize(0),m_blockMin((std::size_t)-1),
      m_blockMax((std::size_t)-1),m_IsX2Present(false)
    {}

    /**
    Workspace based constructor
    @param WA :: Workspace to take pointer
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container>::workspace_iterator(_Container& WA) :
      m_workspace(&WA),m_CPoint(),m_loopCount(1),m_loopOrientation(0),m_index(0),
      m_wsSize(m_workspace->size()),m_blocksize(m_workspace->blocksize()),
      m_blockMin((std::size_t)-1),m_blockMax((std::size_t)-1),m_IsX2Present(false)
    {
      
      m_IsX2Present = isWorkspaceHistogram();
      validateIndex();
    }

    /**
    Multiple loop workspace based constructor
    @param WA :: Workspace to take pointer
    @param loopCount :: The number of time this iterator should loop over the same data before stopping.
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container>::workspace_iterator(_Container& WA, int loopCount) :
      m_workspace(&WA),m_CPoint(),m_loopCount(loopCount),m_loopOrientation(0),m_index(0),
      m_wsSize(m_workspace->size()),m_blocksize(m_workspace->blocksize()),m_blockMin((std::size_t)-1),
      m_blockMax((std::size_t)-1),m_IsX2Present(false)
    {
      
      m_IsX2Present = isWorkspaceHistogram();
      //pretend that the container is longer than it is by multiplying its size by the loopcount
      m_wsSize *= m_loopCount;
      validateIndex();
    }

    /**
    Multiple loop workspace based constructor also specifying the loop orientation
    @param WA :: Workspace to take pointer
    @param loopCount :: The number of time this iterator should loop over the same data before stopping.
    @param loopOrientation :: true = vertical, false = horizontal.
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container>::workspace_iterator(_Container& WA, int loopCount, const unsigned int loopOrientation) :
      m_workspace(&WA),m_CPoint(),m_loopCount(loopCount),m_loopOrientation(loopOrientation),m_index(0),
      m_wsSize(m_workspace->size()),m_blocksize(m_workspace->blocksize()),m_blockMin((std::size_t)-1),
      m_blockMax((std::size_t)-1),m_IsX2Present(false)
    {
      
      m_IsX2Present = isWorkspaceHistogram();
      //pretend that the container is longer than it is by multiplying its size by the loopcount
      m_wsSize *= m_loopCount;
      validateIndex();
    }
    
    /**
    Copy constructor
    @param A :: workspace_iterator to copy
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container>::workspace_iterator(const workspace_iterator<_Iterator, _Container>& A) :
      m_workspace(A.m_workspace),m_CPoint(A.m_CPoint),m_loopCount(A.m_loopCount),m_loopOrientation(A.m_loopOrientation),
      m_index(A.m_index),m_wsSize(A.m_wsSize),m_blocksize(A.m_blocksize),m_blockMin(A.m_blockMin),m_blockMax(A.m_blockMax),
      m_IsX2Present(A.m_IsX2Present),it_dataX(A.it_dataX),it_dataY(A.it_dataY),it_dataE(A.it_dataE)
    {
      validateIndex();
    }

    /**
    Validate the index
    */
    template<typename _Iterator, typename _Container>
    void workspace_iterator<_Iterator, _Container>::validateIndex()
    {
      if (!m_workspace)
        m_index=0;
      else if (m_index>m_wsSize)
        m_index=m_wsSize;
      if (!m_workspace)
      {
        return;
      }

      if (m_index != m_wsSize )
      {
        if (m_index > m_blockMax || m_index < m_blockMin )
        {
          m_dataBlockIndex = m_index/m_blocksize;
          m_blockMin = m_index - (m_index % m_blocksize);
          m_blockMax = m_blockMin + m_blocksize -1;

          //make sure you get the right block if you are looping multiple times
          if (m_loopCount != 1)
          {
            if (m_loopOrientation)
            {
              //vertical Orientation we want to loop over each index value loopcount times.
              m_dataBlockIndex = m_index/(m_blocksize*m_loopCount);
              m_blockMin = m_index - (m_index % (m_blocksize*m_loopCount));
              m_blockMax = m_blockMin + (m_blocksize*m_loopCount) -1;
            }
            else
            {
              //Horizontal Orientation we want to loop over the same datablock loopcount times.
              std::size_t realWsSize = m_wsSize/m_loopCount;
              m_dataBlockIndex = (m_index % realWsSize)/m_blocksize;
            }
          }

          //get cached block level data objects
          it_dataX = m_workspace->dataX(m_dataBlockIndex).begin();
          it_dataY = m_workspace->dataY(m_dataBlockIndex).begin();
          it_dataE = m_workspace->dataE(m_dataBlockIndex).begin();
        }
        size_t iteratorPos;
        if ((m_loopCount != 1) && (m_loopOrientation))
        {
          //vertical Orientation we want to loop over each index value loopcount times.
          // and never change the blockindex
          iteratorPos = (m_index-m_blockMin)/m_loopCount;
        }
        else
        {
          iteratorPos = m_index-m_blockMin;
        }
        // const_cast is needed for the const_iterator (does nothing otherwise)
        m_CPoint.xPointer  = const_cast<double*>(&(it_dataX[iteratorPos]));
        m_CPoint.yPointer = const_cast<double*>(&(it_dataY[iteratorPos]));
        m_CPoint.ePointer  = const_cast<double*>(&(it_dataE[iteratorPos]));
        if(m_IsX2Present)
        {
          m_CPoint.x2Pointer  = const_cast<double*>(&(it_dataX[iteratorPos+1]));
        }
      }
    }
      
    /**
    Addition to index 
    @param N :: Number to add
    @return Iterator advanced by N
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container> workspace_iterator<_Iterator, _Container>::operator+(difference_type N) const
    {
      workspace_iterator<_Iterator, _Container> Out(*this);
      Out+=N;
      return Out;
    }

    /**
    Negation to index
    @param N :: Number to subtract
    @return Iterator decreased by N
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container> workspace_iterator<_Iterator, _Container>::operator-(difference_type N) const
    {
      workspace_iterator<_Iterator, _Container> Out(*this);
      Out-=N;
      return Out;
    }

    /**
    Addition to self by N
    @param N :: Number to add to index
    @return *this
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container>& workspace_iterator<_Iterator, _Container>::operator+=(difference_type N)
    {
      m_index+=N;
      validateIndex();
      return *this;
    }

    /**
    Negation to self by N
    @param N :: Number to subtract
    @return *this
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container>& workspace_iterator<_Iterator, _Container>::operator-=(difference_type N)
    {
      m_index-=N;
      validateIndex();
      return *this;
    }

    /**
    Increment iterator (pre)
    @return Iterator
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container>& workspace_iterator<_Iterator, _Container>::operator++()
    {
      ++m_index;
      validateIndex();
      return *this;
    }

    /**
    Decrement iterator (pre)
    @return Iterator 
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container>& workspace_iterator<_Iterator, _Container>::operator--()
    {
      --m_index;
      validateIndex();
      return *this;
    }

    /**
    Increment iterator (post)
    @return Iterator before increment
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container> workspace_iterator<_Iterator, _Container>::operator++(int) 
    {
      workspace_iterator<_Iterator, _Container> Out(*this);
      this->operator++();
      return Out;
    }

    /**
    Negation iterator (post)
    @return Iterator before decrement
    */
    template<typename _Iterator, typename _Container>
    workspace_iterator<_Iterator, _Container> workspace_iterator<_Iterator, _Container>::operator--(int) 
    {
      workspace_iterator<_Iterator, _Container> Out(*this);
      this->operator--();
      return Out;
    }

    /**
    Difference iterator
    @return difference (as a non-inclusive count)
    */
    template<typename _Iterator, typename _Container>
    typename std::iterator_traits<_Iterator*>::difference_type workspace_iterator<_Iterator, _Container>::operator-(const workspace_iterator<_Iterator, _Container>& A) const
    {
      if (!m_workspace && !A.m_workspace)
        return 0;
      if (!m_workspace)                      /// This effectively an end
        return A.m_wsSize-A.m_index;
      if (!A.m_workspace)                    /// A effectively an end
        return m_index-m_wsSize;
      return A.m_index-m_index;
    }

    template<typename _Iterator, typename _Container>
    bool workspace_iterator<_Iterator, _Container>::isWorkspaceHistogram()
    {
      if (m_wsSize > 0)
      {
        return (m_workspace->dataX(0).size() > m_workspace->dataY(0).size());
      }
      return false;
    }


  }  // NAMESPACE API

}  // NAMESPACE Mantid
