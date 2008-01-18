#include <vector>
#include <iterator>
#include <boost/shared_ptr.hpp>

#include "TripleRef.h"

namespace Mantid 
{
  namespace API
  {
    /*!
    Null constructor
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container>::triple_iterator() :
      m_workspace(0),m_CPoint(),m_index(0),m_wsSize(0),m_blocksize(0),m_blockMin(-1),m_blockMax(-1)
    {}

    /*!
    Workspace based constructor
    \param WA :: Workspace to take pointer
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container>::triple_iterator(_Container& WA) :
      m_workspace(&WA),m_CPoint(),m_index(0),
      m_wsSize(m_workspace->size()),m_blocksize(m_workspace->blocksize()),m_blockMin(-1),m_blockMax(-1)
    {
      validateIndex();
    }
    
    /*!
    Copy constructor
    \param A :: triple_iterator to copy
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container>::triple_iterator(const triple_iterator<_Iterator, _Container>& A) :
      m_workspace(A.m_workspace),m_CPoint(),m_index(A.m_index),m_wsSize(A.m_wsSize),
      m_blocksize(A.m_blocksize),m_blockMin(A.m_blockMin),m_blockMax(A.m_blockMax),
      it_dataX(A.it_dataX),it_dataY(A.it_dataY),it_dataE(A.it_dataE)
    {
      validateIndex();
    }

    /*!
    Validate the index
    */
    template<typename _Iterator, typename _Container>
    void triple_iterator<_Iterator, _Container>::validateIndex()
    {
      if (m_index<0 || !m_workspace)
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

          it_dataX = m_workspace->dataX(m_dataBlockIndex).begin();
          it_dataY = m_workspace->dataY(m_dataBlockIndex).begin();
          it_dataE = m_workspace->dataE(m_dataBlockIndex).begin();
        }
        m_CPoint.first  = &(it_dataX[m_index-m_blockMin]);
        m_CPoint.second = &(it_dataY[m_index-m_blockMin]);
        m_CPoint.third  = &(it_dataE[m_index-m_blockMin]);
      }
    }
      
    /*!
    Addition to index 
    \param N :: Number to add
    \return Iterator advanced by N
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container> triple_iterator<_Iterator, _Container>::operator+(difference_type N) const
    {
      triple_iterator<_Iterator, _Container> Out(*this);
      Out+=N;
      return Out;
    }

    /*!
    Negation to index
    \param N :: Number to subtract
    \return Iterator decreased by N
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container> triple_iterator<_Iterator, _Container>::operator-(difference_type N) const
    {
      triple_iterator<_Iterator, _Container> Out(*this);
      Out-=N;
      return Out;
    }

    /*!
    Addition to self by N
    \param N :: Number to add to index
    \return *this
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container>& triple_iterator<_Iterator, _Container>::operator+=(difference_type N)
    {
      m_index+=N;
      validateIndex();
      return *this;
    }

    /*!
    Negation to self by N
    \param N :: Number to subtract
    \return *this
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container>& triple_iterator<_Iterator, _Container>::operator-=(difference_type N)
    {
      m_index-=N;
      validateIndex();
      return *this;
    }

    /*!
    Increment iterator (pre)
    \return Iterator
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container>& triple_iterator<_Iterator, _Container>::operator++()
    {
      ++m_index;
      validateIndex();
      return *this;
    }

    /*!
    Decrement iterator (pre)
    \return Iterator 
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container>& triple_iterator<_Iterator, _Container>::operator--()
    {
      --m_index;
      validateIndex();
      return *this;
    }

    /*!
    Increment iterator (post)
    \return Iterator before increment
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container> triple_iterator<_Iterator, _Container>::operator++(int) 
    {
      triple_iterator<_Iterator, _Container> Out(*this);
      this->operator++();
      return Out;
    }

    /*!
    Negation iterator (post)
    \return Iterator before decrement
    */
    template<typename _Iterator, typename _Container>
    triple_iterator<_Iterator, _Container> triple_iterator<_Iterator, _Container>::operator--(int) 
    {
      triple_iterator<_Iterator, _Container> Out(*this);
      this->operator--();
      return Out;
    }

    /*!
    Difference iterator
    \return difference (as a non-inclusive count)
    */
    template<typename _Iterator, typename _Container>
    typename triple_iterator<_Iterator, _Container>::difference_type triple_iterator<_Iterator, _Container>::operator-(const triple_iterator<_Iterator, _Container>& A) const
    {
      if (!m_workspace && !A.m_workspace)
        return 0;
      if (!m_workspace)                      /// This effectively an end
        return A.m_wsSize-A.m_index;
      if (!A.m_workspace)                    /// A effectively an end
        return m_index-m_wsSize;
      return A.m_index-m_index;
    }


  }  // NAMESPACE API

}  // NAMESPACE Mantid
