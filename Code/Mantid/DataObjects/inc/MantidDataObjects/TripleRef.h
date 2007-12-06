#ifndef TripleRef_h
#define TripleRef_h

#include "MantidKernel/System.h"

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
  TripleRef of three identical types.
  Class maintians a type first/second/third triple
  similar to std::pair except all are identical

  \author S. Ansell
  \date April 2005
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
template<typename T>
class DLLExport TripleRef
{
  public:
  
  T first;       ///< First item
  T second;      ///< Second item
  T third;       ///< Third item

  TripleRef(T A,T B,T C);
  TripleRef(const TripleRef<T>&);
  TripleRef<T>& operator=(const TripleRef<T>&);
  ~TripleRef();

  T operator[](int const) const;
  T operator[](int const);
  int operator<(const TripleRef<T>&) const;
  int operator>(const TripleRef<T>&) const;
  int operator==(const TripleRef<T>&) const;
  int operator!=(const TripleRef<T>&) const;

};

}  // NAMESPACE Iterator

}  // NAMESPACE Mantid

#endif
