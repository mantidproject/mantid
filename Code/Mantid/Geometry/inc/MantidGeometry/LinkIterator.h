#ifndef LinkIterator_h
#define LinKIterator_h

/*!
  The LinkIterator deals with a set of identical
  containers that need to be iterated over.
  This adds a number of components and provides
  a "uniform" iterator.

  \class LinkIterator
  \author S. Ansell
  \version 1.0
  \date August 2007
  \brief This stores a group of iterators

  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
template<template<typename T> class V,typename T> 
class LinkIterator
{
 private:
    
  int index;                           ///< Position in veclist

  typename V<T>::iterator ptIter;       ///< Actual interator
  std::vector<typename V<T>::iterator> begItems;   ///< Iterator pointing to first item
  std::vector<typename V<T>::iterator> endItems;   ///< Iterator pointing one-past-the-end
    
 public:

  LinkIterator();
  LinkIterator(const LinkIterator&);
  LinkIterator& operator=(const LinkIterator&);
  ~LinkIterator() {}   ///< Destructor
  
  void addComponent(const typename V<T>::iterator&,
		    const typename V<T>::iterator&);

  void init();

  bool operator==(const LinkIterator&) const;
  bool operator!=(const LinkIterator&) const;
  int distance(const LinkIterator&) const;    ///< Not implemented

  T& operator*() { return *ptIter; }              ///< Dereference operator
  const T& operator*() const { return *ptIter; }  ///< Dereference operator (const version)
  T& operator->() { return *ptIter; }             ///< Overload of -> operator
  const T& operator->() const { return *ptIter; } ///< Overload of -> operator (const version)
  
  typename V<T>::iterator operator++();
  typename V<T>::iterator operator++(const int i);

  typename V<T>::iterator operator--();           ///< Not implemented
  typename V<T>::iterator operator--(const int);  ///< Not implemented

  /// Determine if at end of iteration
  bool isEnd() const
    {
      return (endItems.empty() || ptIter==endItems.back()); 
    }

  /// Get Actual end iterator value
  const typename V<T>::iterator end() const
     {
       return (endItems.empty()) ? 0 : endItems.back();
     } 

};


/*!
  Default constructor
*/
template<template<typename T> class V, typename T>
LinkIterator<V,T>::LinkIterator() :
  index(0),ptIter(0)
{}

/*!
  Copy constructor
  \param A :: LinkIterator to copy
 */
template<template<typename T> class V, typename T>
LinkIterator<V,T>::LinkIterator(const LinkIterator& A) :
  index(A.index),ptIter(A.ptIter),
  begItems(A.begItems),endItems(A.endItems)
{}

/*!
  Assignment operator
  \param A :: LinkIterator to copy
  \return *this
 */
template<template<typename T> class V, typename T>
LinkIterator<V,T>&
LinkIterator<V,T>::operator=(const LinkIterator<V,T>& A) 
{
  if (this!=&A)
    {
      index=A.index;
      ptIter=A.ptIter;
      begItems=A.begItems;
      endItems=A.endItems;
    }
  return *this;
}

// -------------------------------------------------
//                OPERATORS
// -------------------------------------------------


/*!
  Equality operator.
  Uses the iterator only
  \param A :: LinkIteator to compare
 */
template<template<typename T> class V, typename T>
bool 
LinkIterator<V,T>::operator==(const LinkIterator& A) const
{
  return (ptIter==A.ptIter); 
}

/*!
  Equality operator.
  Uses the iterator only
  \param A :: LinkIteator to compare
 */
template<template<typename T> class V, typename T>
bool 
LinkIterator<V,T>::operator!=(const LinkIterator& A) const
{
  return (ptIter!=A.ptIter); 
}
    
/*!
  Increment operator.
  This currently doesnt throw but could?
  \param i Does nothing with this argument
  \return True Iterator 
 */
template<template<typename T> class V, typename T>
typename V<T>::iterator 
LinkIterator<V,T>::operator++(const int i)
{
  typename V<T>::iterator retVal=ptIter;
  this->operator++();
  return retVal;
}

/*!
  Increment operator.
  This currently doesnt throw but could?
  \return True Iterator 
 */
template<template<typename T> class V, typename T>
typename  V<T>::iterator 
LinkIterator<V,T>::operator++()
{
  if (ptIter==endItems.back())
    return ptIter;

  ptIter++;
  if (ptIter==endItems[index])   // hit an end stop
    {
      if (ptIter==endItems.back())   // can't go past last placeholder
	return ptIter;
      index++;
      ptIter=begItems[index];
    }
  return ptIter;
}

// -----------------------------------------
//                  METHODS
// --------------------------------------

/*!
  Set to first item
 */
template<template<typename T> class V, typename T>
void
LinkIterator<V,T>::init()
{
  if (!begItems.empty())
    {
      ptIter=begItems.front();
      index=0;
    }
  return;
}

/*!
  Two iterators are added to the link list
  \param begPt :: Begining iterator
  \param endPt :: End iterator
 */
template<template<typename T> class V, typename T>
void
LinkIterator<V,T>::addComponent(const typename V<T>::iterator& begPt,
				const typename V<T>::iterator& endPt)
{
  if (begItems.empty())
    {
      ptIter=begPt;
      index=0;
    }

  begItems.push_back(begPt);
  endItems.push_back(endPt);
  return;
}
      


#endif
