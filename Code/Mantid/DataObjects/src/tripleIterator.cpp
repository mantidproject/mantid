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
/*!
  Null constructor
*/
template<typename WorkSpace>
triple_iterator<WorkSpace>::triple_iterator() :
  W(0),CPoint(0),index(0)
{}

/*!
  Workspace based constructor
  \param WA :: Workspace to take pointer
*/
template<typename WorkSpace>
triple_iterator<WorkSpace>::triple_iterator(WorkSpace& WA) :
  W(&WA),CPoint(0),index(0)
{
  validateIndex();
}

/*!
  Copy constructor
  \param A :: triple_iterator to copy
*/
template<typename WorkSpace>
triple_iterator<WorkSpace>::triple_iterator(const triple_iterator<WorkSpace>& A) :
  W(A.W),CPoint(0),index(A.index)
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
  else if (index>W->size())
    index=W->size();
  delete CPoint;
  CPoint=(W && index!=W->size()) ? 
    new TripleRef<double&>(W->dataX()[index],W->dataY()[index],W->dataE()[index]) : 0;
  return;
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
  \return differen
*/
template<typename WorkSpace>
typename triple_iterator<WorkSpace>::difference_type
triple_iterator<WorkSpace>::operator-(const triple_iterator<WorkSpace>& A) const
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
