#ifndef triple_iterator_h
#define triple_iterator_h

namespace Mantid
{

namespace Iterator
{

template<typename WorkSpace>
class triple_iterator : public std::iterator<std::random_access_iterator_tag,TripleRef<double&>,int,
					     TripleRef<double&>*,TripleRef<double&>& >
{
 private:
  
  WorkSpace* W;
  TripleRef<double&>* CPoint;
  int index;

  void validateIndex(); 

 public:

/*
  typedef TripleRef<double> value_type;
  typedef TripleRef<double>& reference;
  typedef TripleRef<double>* pointer;
  typedef int difference_type;
  typedef std::random_iterator_tag iterator_category;
*/

  triple_iterator<WorkSpace>();
  triple_iterator<WorkSpace>(WorkSpace&);
  triple_iterator<WorkSpace>(const triple_iterator<WorkSpace>&);
  
  const TripleRef<double&>& operator*() const { return *CPoint; }   ///< Base Accessor
  const TripleRef<double&>* operator->() const { return CPoint; }   ///< Base Pointer accessor
  const TripleRef<double&>& operator[](int) const; 

  triple_iterator<WorkSpace>& operator++();
  triple_iterator<WorkSpace> operator++(int);
  triple_iterator<WorkSpace>& operator--();
  triple_iterator<WorkSpace> operator--(int);
  triple_iterator<WorkSpace>& operator+=(difference_type);
  triple_iterator<WorkSpace>& operator-=(difference_type);
  triple_iterator<WorkSpace> operator+(difference_type) const;
  triple_iterator<WorkSpace> operator-(difference_type) const;
  difference_type operator-(const triple_iterator<WorkSpace>&) const;


  bool 
  operator==(const triple_iterator<WorkSpace>& A)  const
  /*!
    Equality operator
    \param A :: Iterator to compare
    \return equality status
   */
  { 
    if (!W)
      {
	if (!A.W) return 1;
	return  (A.W->size()==A.index) ? 1 : 0;
      }
    if (!A.W)
      return  (W->size()==index) ? 1 : 0;
    
    return (index==A.index); 
  }

  bool operator!=(const triple_iterator<WorkSpace>& A)  const
  /*!
    InEquality operator
    \param A :: Iterator to compare
    \return equality status
   */
    {  return (!(operator==(A))); }


  triple_iterator<WorkSpace> begin() const
  /*!
    Begin iterator (Effective copy+set zero)
    \return beginning iterator
   */
    { 
      triple_iterator<WorkSpace> Out(*this);
      Out.index=0;
      Out.validateIndex();
      return Out;
    }

  triple_iterator<WorkSpace>& end() const 
  /*!
    End iterator
    \return Null/end iterator
  */
    { 
      static triple_iterator<WorkSpace> endIter;
      return endIter; 
    }

};

}  // NAMESPACE Iterator

}  // NAMESPACE Mantid

#endif
