#ifndef TripleRef_h
#define TripleRef_h

namespace Mantid
{

/*!
  \namespace Iterator
  \author S. Ansell
  \version 0.1
  \date December 2007
  \brief Holds items to allow iteration on data types
*/
  
namespace Iterator
{

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

  TripleRef(const T A,const T B,const T C);
  TripleRef(const TripleRef<T>&);
  TripleRef<T>& operator=(const TripleRef<T>&);
  ~TripleRef();

  const T operator[](int const) const;
  T operator[](int const);
  int operator<(const TripleRef<T>&) const;
  int operator>(const TripleRef<T>&) const;
  int operator==(const TripleRef<T>&) const;
  int operator!=(const TripleRef<T>&) const;

};

}  // NAMESPACE Iterator

}  // NAMESPACE Mantid

#endif
