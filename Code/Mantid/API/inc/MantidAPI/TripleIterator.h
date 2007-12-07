#ifndef MANTIDAPI_TRIPLE_ITERATOR_H
#define MANTIDAPI_TRIPLE_ITERATOR_H
#include "MantidKernel/System.h"


namespace Mantid
{

namespace API
{
/*!
  triple_iterator iterates over a workspace providing values as TripleRefs

  \class triple_iterator
  \author S. Ansell
  \date November 2007
  \version 1.0 
    
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
  
  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/
template<typename WorkSpace>
class DLLExport triple_iterator : public std::iterator<std::random_access_iterator_tag,TripleRef<double&>,int,
					     TripleRef<double&>*,TripleRef<double&>& >
{
 private:
  ///internal workspace pointer
  WorkSpace* W;
  /// pointer to a TripleRef of doubles
  TripleRef<double&>* CPoint;
  /// internal index of location within the workspace
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

  TripleRef<double&>& operator*() { return *CPoint; }   ///< Base Accessor
  TripleRef<double&>* operator->() { return CPoint; }   ///< Base Pointer accessor
  
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

}  // NAMESPACE API

}  // NAMESPACE Mantid

#endif //MANTIDAPI_TRIPLE_ITERATOR_H
