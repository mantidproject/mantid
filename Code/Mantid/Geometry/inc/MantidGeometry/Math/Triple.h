#ifndef Triple_h
#define Triple_h
#include "MantidKernel/System.h"

namespace Mantid
{
/*!
  \class Triple
  \brief Triple of three identical types
  \author S. Ansell
  \date April 2005
  \version 1.0
  
  Class maintians a type first/second/third triple
  similar to std::pair except all are identical

  Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport Triple
{
  public:
  
  T first;       ///< First item
  T second;      ///< Second item
  T third;       ///< Third item

  Triple();
  Triple(const Triple<T>&);
  Triple(const T&,const T&,const T&);
  Triple<T>& operator=(const Triple<T>&);
  ~Triple();

  T operator[](const int A) const;
  T& operator[](const int A);
  int operator<(const Triple<T>&) const;
  int operator>(const Triple<T>&) const;
  int operator==(const Triple<T>&) const;
  int operator!=(const Triple<T>&) const;

};


/*!
  \class DTriple
  \brief Triple of three different things
  \author S. Ansell
  \date April 2005
  \version 1.0
  
  Class maintians a different type first/second/third triple
  All are of a different type
*/

template<typename F,typename S,typename T>
class DLLExport DTriple 
{
  public:
  
  F first;         ///< First item
  S second;        ///< Second item
  T third;         ///< Third item

  DTriple();
  DTriple(const DTriple<F,S,T>&);
  DTriple(const F&,const S&,const T&);
  DTriple<F,S,T>& operator=(const DTriple<F,S,T>&);
  ~DTriple();

  int operator<(const DTriple<F,S,T>&) const;
  int operator>(const DTriple<F,S,T>&) const;
  int operator==(const DTriple<F,S,T>&) const;
  int operator!=(const DTriple<F,S,T>&) const;

};
}  // NAMESPACE Mantid
#endif
