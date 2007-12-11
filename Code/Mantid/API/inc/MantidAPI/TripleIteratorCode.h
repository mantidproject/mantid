#include <vector>
#include <iterator>
#include <boost/shared_ptr.hpp>

#include "MantidAPI/TripleRef.h"

namespace Mantid 
{

namespace API
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
  if (!W)
    {
      CPoint=0;
      return;
    }

  //work out the datalock to get
  const int dataBlockIndex(index/W->blocksize());
  const int itemIndex(index % W->blocksize());

  CPoint=(index!=W->size()) ? 
    new TripleRef<double&>( W->dataX(dataBlockIndex)[itemIndex],
                            W->dataY(dataBlockIndex)[itemIndex],
                            W->dataE(dataBlockIndex)[itemIndex]
                           ) : 0;
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
