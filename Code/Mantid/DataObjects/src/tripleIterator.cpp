#include <fstream>
#include <iostream>
#include <iomanip>
#include <list>
#include <vector>
#include <iterator>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/RefControl.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidDataObjects/TripleRef.h"
#include "MantidDataObjects/tripleIterator.h"


namespace Mantid 
{

namespace Iterator
{

template<typename WorkSpace>
triple_iterator<WorkSpace>::triple_iterator() :
  W(0),CPoint(0),index(0)
  /*!
    Null constructor
  */
{}

template<typename WorkSpace>
triple_iterator<WorkSpace>::triple_iterator(WorkSpace& WA) :
  W(&WA),CPoint(0),index(0)
  /*!
    Workspace based constructor
    \param A :: Workspace to take pointer
  */
{
  validateIndex();
}

template<typename WorkSpace>
triple_iterator<WorkSpace>::triple_iterator(const triple_iterator<WorkSpace>& A) :
  W(A.W),CPoint(0),index(A.index)
  /*!
    Copy constructor
    \param A :: triple_iterator to copy
  */
{
  validateIndex();
}

template<typename WorkSpace>
void
triple_iterator<WorkSpace>::validateIndex()
  /*!
    Validate the index
  */
{
  if (index<0 || !W)
    index=0;
  else if (index>W->size())
    index=W->size();
  delete CPoint;
  CPoint=(W && index!=W->size()) ? 
    new TripleRef<double&>(W->dataX()[index],W->dataY()[index],W->dataE()[index]) : 0;
  return;
}


template<typename WorkSpace>
triple_iterator<WorkSpace>
triple_iterator<WorkSpace>::operator+(int N) const
  /*!
    Addition to index 
    \param N :: Number to add
    \return Iterator advanced by N
  */
{
  triple_iterator<WorkSpace> Out(*this);
  Out+=N;
  return Out;
}

template<typename WorkSpace>
triple_iterator<WorkSpace>
triple_iterator<WorkSpace>::operator-(int N) const
  /*!
    Negation to index
    \param N :: Number to subtrct
    \return Iterator decreased by N
  */
{
  triple_iterator<WorkSpace> Out(*this);
  Out-=N;
  return Out;
}


template<typename WorkSpace>
triple_iterator<WorkSpace>&
triple_iterator<WorkSpace>::operator+=(int N)
  /*!
    Addition to self by N
    \param N :: Number to add to index
    \return *this
  */
{
  index+=N;
  validateIndex();
  return *this;
}

template<typename WorkSpace>
triple_iterator<WorkSpace>&
triple_iterator<WorkSpace>::operator-=(int N)
  /*!
    Negation to self by N
    \param N :: Number to subtract
    \return *this
  */
{
  index-=N;
  validateIndex();
  return *this;
}

template<typename WorkSpace>
triple_iterator<WorkSpace>&
triple_iterator<WorkSpace>::operator++()
  /*!
    Increment iterator (pre)
    \return Iterator
  */
{
  index++;
  validateIndex();
  return *this;
}

template<typename WorkSpace>
triple_iterator<WorkSpace>&
triple_iterator<WorkSpace>::operator--()
  /*!
    Decrement iterator (pre)
    \return Iterator 
  */
{
  index--;
  validateIndex();
  return *this;
}

template<typename WorkSpace>
triple_iterator<WorkSpace>
triple_iterator<WorkSpace>::operator++(int) 
  /*!
    Increment iterator (post)
    \return Iterator before increment
  */
{
  triple_iterator<WorkSpace> Out(*this);
  this->operator++();
  return Out;
}

template<typename WorkSpace>
triple_iterator<WorkSpace>
triple_iterator<WorkSpace>::operator--(int) 
  /*!
    Negation iterator (post)
    \return Iterator before decrement
  */
{
  triple_iterator<WorkSpace> Out(*this);
  this->operator--();
  return Out;
}

template<typename WorkSpace>
typename triple_iterator<WorkSpace>::difference_type
triple_iterator<WorkSpace>::operator-(const triple_iterator<WorkSpace>& A) const
  /*!
    Difference iterator
    \return differen
  */
{
  if (!W && !A.W)
    return 0;
  if (!W)
    return A.index-A.W->size();
  if (!A.W)
    return W->size()-index;
  return A.index-index;
}

///\cond TEMPLATE


template class triple_iterator<DataObjects::Histogram1D>;

///\endcond TEMPLATE

}  // NAMESPACE Iterator

}  // NAMESPACE Mantid
