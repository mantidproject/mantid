#include <vector>
#include <iterator>
#include <boost/shared_ptr.hpp>
#include <iostream>

#include "TripleRef.h"

namespace Mantid 
{

  namespace API
  {
    /*!
    Null constructor
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>::triple_iterator() :
    W(0),CPoint(),index(0),wsSize(0),blocksize(0),blockMin(-1),blockMax(-1)
    {}

    /*!
    Workspace based constructor
    \param WA :: Workspace to take pointer
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>::triple_iterator(WorkSpace& WA) :
    W(&WA),CPoint(),index(0),wsSize(W->size()),blocksize(W->blocksize()),blockMin(-1),blockMax(-1)
    {
      validateIndex();
    }

    /*!
    Copy constructor
    \param A :: triple_iterator to copy
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>::triple_iterator(const triple_iterator<WorkSpace>& A) :
    W(A.W),CPoint(),index(A.index),wsSize(A.wsSize),blocksize(A.blocksize),
      blockMin(A.blockMin),blockMax(A.blockMax),
      it_dataX(A.it_dataX),it_dataY(A.it_dataY),it_dataE(A.it_dataE)
    {
      validateIndex();

    }

    /*!
    Validate the index
    */
    template<typename WorkSpace>
    void
      triple_iterator<WorkSpace>::validateIndex()
    {
      if (index<0 || !W)
        index=0;
      else if (index>wsSize)
        index=wsSize;
      if (!W)
      {
        return;
      }

      if (index != wsSize )
      {
        if (index > blockMax || index < blockMin )
        {
          dataBlockIndex = index/blocksize;
          blockMin = index - (index % blocksize);
          blockMax = blockMin + blocksize -1;

          it_dataX = W->dataX(dataBlockIndex).begin();
          it_dataY = W->dataY(dataBlockIndex).begin();
          it_dataE = W->dataE(dataBlockIndex).begin();
        }
        CPoint.first  = &(it_dataX[index-blockMin]);
        CPoint.second = &(it_dataY[index-blockMin]);
        CPoint.third  = &(it_dataE[index-blockMin]);
      }
    }

    /*!
    Addition to index 
    \param N :: Number to add
    \return Iterator advanced by N
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>
      triple_iterator<WorkSpace>::operator+(difference_type N) const
    {
      triple_iterator<WorkSpace> Out(*this);
      Out+=N;
      return Out;
    }

    /*!
    Negation to index
    \param N :: Number to subtract
    \return Iterator decreased by N
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>
      triple_iterator<WorkSpace>::operator-(difference_type N) const
    {
      triple_iterator<WorkSpace> Out(*this);
      Out-=N;
      return Out;
    }

    /*!
    Addition to self by N
    \param N :: Number to add to index
    \return *this
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>&
      triple_iterator<WorkSpace>::operator+=(difference_type N)
    {
      index+=N;
      validateIndex();
      return *this;
    }

    /*!
    Negation to self by N
    \param N :: Number to subtract
    \return *this
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>&
      triple_iterator<WorkSpace>::operator-=(difference_type N)
    {
      index-=N;
      validateIndex();
      return *this;
    }

    /*!
    Increment iterator (pre)
    \return Iterator
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>&
      triple_iterator<WorkSpace>::operator++()
    {
      index++;
      validateIndex();
      return *this;
    }

    /*!
    Decrement iterator (pre)
    \return Iterator 
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>&
      triple_iterator<WorkSpace>::operator--()
    {
      index--;
      validateIndex();
      return *this;
    }

    /*!
    Increment iterator (post)
    \return Iterator before increment
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>
      triple_iterator<WorkSpace>::operator++(int) 
    {
      triple_iterator<WorkSpace> Out(*this);
      this->operator++();
      return Out;
    }

    /*!
    Negation iterator (post)
    \return Iterator before decrement
    */
    template<typename WorkSpace>
    triple_iterator<WorkSpace>
      triple_iterator<WorkSpace>::operator--(int) 
    {
      triple_iterator<WorkSpace> Out(*this);
      this->operator--();
      return Out;
    }

    /*!
    Difference iterator
    \return difference (as a non-inclusive count)
    */
    template<typename WorkSpace>
    typename triple_iterator<WorkSpace>::difference_type
      triple_iterator<WorkSpace>::operator-(const triple_iterator<WorkSpace>& A) const
    {
      if (!W && !A.W)
        return 0;
      if (!W)                      /// This effectively an end
        return A.W->size()-A.index;
      if (!A.W)                    /// A effectively an end
        return index-W->size();
      return A.index-index;
    }


  }  // NAMESPACE API

}  // NAMESPACE Mantid
