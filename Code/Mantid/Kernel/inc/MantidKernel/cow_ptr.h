#ifndef MANTIDKERNEL_COW_PTR_H
#define MANTIDKERNEL_COW_PTR_H

#include "boost/shared_ptr.hpp"
#include "MultiThreaded.h"

namespace Mantid
{
namespace Kernel
{

/*!
  \class cow_ptr
  \brief Impliments a copy on write data template 
  \version 1.0
  \date February 2006
  \author S.Ansell
  
  This version works only on data that is created via new().
  It is thread safe and works in the Standard template 
  libraries (but appropiate functionals are needed for 
  sorting etc.).

  Renamed from RefControl on the 11/12/2007, 
  as it was agreed that copy on write pointer better 
  described the functionality of this class.

  The underlying data can be accessed via the normal pointer
  semantics but call the access function if the data is required
  to be modified.

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

template<typename DataType>
class cow_ptr
{
 public:

  typedef boost::shared_ptr<DataType> ptr_type;   ///< typedef for the storage
  typedef DataType value_type;                    ///< typedef for the data type

 private:

  ptr_type Data;                                  ///< Real object Ptr

 public:

  cow_ptr();
  cow_ptr(const cow_ptr<DataType>&);      
  cow_ptr<DataType>& operator=(const cow_ptr<DataType>&);
  cow_ptr<DataType>& operator=(const ptr_type&);
  ~cow_ptr();

  const DataType& operator*() const { return *Data; }  ///< Pointer dereference access
  const DataType* operator->() const { return Data.get(); }  ///<indirectrion dereference access
  bool operator==(const cow_ptr<DataType>& A) { return Data==A.Data; } ///< Based on ptr equality
  DataType& access();

};

template<typename DataType>
cow_ptr<DataType>::cow_ptr() :
  Data(new DataType())
  /*!
    Constructor : creates new data() object
  */
{ }


template<typename DataType>
cow_ptr<DataType>::cow_ptr(const cow_ptr<DataType>& A) :
  Data(A.Data)
  /*!
    Copy constructor : double references the data object
    \param A :: object to copy
  */
{ }

template<typename DataType>
cow_ptr<DataType>&
cow_ptr<DataType>::operator=(const cow_ptr<DataType>& A) 
  /*!
    Assignment operator : double references the data object
    maybe drops the old reference.
    \param A :: object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      Data=A.Data;
    }
  return *this;
}

template<typename DataType>
cow_ptr<DataType>&
cow_ptr<DataType>::operator=(const ptr_type& A) 
  /*!
    Assignment operator : double references the data object
    maybe drops the old reference.
    \param A :: object to copy
    \return *this
  */
{
  if (this->Data != A)
    {
      Data=A;
    }
  return *this;
}


template<typename DataType>
cow_ptr<DataType>::~cow_ptr()
  /*!
    Destructor : No work is required since Data is
    a shared_ptr.
  */
{}

template<typename DataType>
DataType&
cow_ptr<DataType>::access()
  /*!
    Access function 
    Creates a copy of Data so that it can be modified.
    Believed to be thread safe sicne
    creates an extra reference before deleteing.
    \return new copy of *this
  */
{
  if (Data.unique())
    return *Data;

  ptr_type oldData=Data; 
  Data.reset();
  Data=ptr_type(new DataType(*oldData));

  return *Data;
}

} // NAMESPACE Kernel
} // NAMESPACE Mantid

#endif //MANTIDKERNEL_COW_PTR_H
