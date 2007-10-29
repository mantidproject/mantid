#ifndef LinkIterator_h
#define LinKIterator_h

/*!
  \class LinkIterator
  \author S. Ansell
  \version 1.0
  \date August 2007
  \brief This stores a group of iterators

  The LinkIterator deals with a set of identical
  containers that need to be iterated over.
  This adds a number of components and provides
  a "uniform" iterator.
 */

template<template<typename T> class V,typename T> 
class LinkIterator
{
 private:
    
  int index;                           ///< Position in veclist

  typename V<T>::iterator ptIter;       ///< Actual interator
  std::vector<typename V<T>::iterator> begItems;
  std::vector<typename V<T>::iterator> endItems;
    
 public:

  LinkIterator();
  LinkIterator(const LinkIterator&);
  LinkIterator& operator=(const LinkIterator&);
  ~LinkIterator() {}
  
  void addComponent(const typename V<T>::iterator&,
		    const typename V<T>::iterator&);

  void init();

  bool operator==(const LinkIterator&) const;
  bool operator!=(const LinkIterator&) const;
  int distance(const LinkIterator&) const;

  T& operator*() { return *ptIter; }
  const T& operator*() const { return *ptIter; }
  T& operator->() { return *ptIter; }
  const T& operator->() const { return *ptIter; }
  
  typename V<T>::iterator operator++();
  typename V<T>::iterator operator++(const int);

  typename V<T>::iterator operator--();
  typename V<T>::iterator operator--(const int);

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


template<template<typename T> class V, typename T>
LinkIterator<V,T>::LinkIterator() :
  index(0),ptIter(0)
  /*!
    Default constructor
  */
{}

template<template<typename T> class V, typename T>
LinkIterator<V,T>::LinkIterator(const LinkIterator& A) :
  index(A.index),ptIter(A.ptIter),
  begItems(A.begItems),endItems(A.endItems)
  /*!
    Copy constructor
    \param A :: LinkIterator to copy
   */
{}

template<template<typename T> class V, typename T>
LinkIterator<V,T>&
LinkIterator<V,T>::operator=(const LinkIterator<V,T>& A) 
  /*!
    Assignment operator
    \param A :: LinkIterator to copy
    \return *this
   */
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


template<template<typename T> class V, typename T>
bool 
LinkIterator<V,T>::operator==(const LinkIterator& A) const
  /*!
    Equality operator.
    Uses the iterator only
    \param A :: LinkIteator to compare
   */
{
  return (ptIter==A.ptIter); 
}

template<template<typename T> class V, typename T>
bool 
LinkIterator<V,T>::operator!=(const LinkIterator& A) const
  /*!
    Equality operator.
    Uses the iterator only
    \param A :: LinkIteator to compare
   */
{
  return (ptIter!=A.ptIter); 
}
    
template<template<typename T> class V, typename T>
typename V<T>::iterator 
LinkIterator<V,T>::operator++(const int)
  /*!
    This currently doesnt throw but could?
    \return True Iterator 
   */
{
  typename V<T>::iterator retVal=ptIter;
  this->operator++();
  return retVal;
}

template<template<typename T> class V, typename T>
typename  V<T>::iterator 
LinkIterator<V,T>::operator++()
  /*!
    This currently doesnt throw but could?
    \return True Iterator 
   */
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

template<template<typename T> class V, typename T>
void
LinkIterator<V,T>::init()
  /*!
    Set to first item
   */
{
  if (!begItems.empty())
    {
      ptIter=begItems.front();
      index=0;
    }
  return;
}

template<template<typename T> class V, typename T>
void
LinkIterator<V,T>::addComponent(const typename V<T>::iterator& begPt,
				const typename V<T>::iterator& endPt)
  /*!
    Two iterators are added to the link list
    \param begPt :: Begining iterator
    \param endPt :: End iterator
   */
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
