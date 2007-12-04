#ifndef TripleRef_h
#define TripleRef_h

/*!
  \class TripleRef
  \brief TripleRef of three identical types
  \author S. Ansell
  \date April 2005
  \version 1.0
  
  Class maintians a type first/second/third triple
  similar to std::pair except all are identical
*/

template<typename T>
class TripleRef
{
  public:
  
  T first;       ///< First item
  T second;      ///< Second item
  T third;       ///< Third item

  TripleRef(const T,const T,const T);
  TripleRef(const TripleRef<T>&);
  TripleRef<T>& operator=(const TripleRef<T>&);
  ~TripleRef();

  const T operator[](const int) const;
  T operator[](const int);
  T operator[](const int);
  int operator<(const TripleRef<T>&) const;
  int operator>(const TripleRef<T>&) const;
  int operator==(const TripleRef<T>&) const;
  int operator!=(const TripleRef<T>&) const;

};


#endif
