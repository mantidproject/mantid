#ifndef MANTIDKERNEL_COW_PTR_H
#define MANTIDKERNEL_COW_PTR_H

#include "MultiThreaded.h"

#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif

#include <vector>

namespace Mantid
{
namespace Kernel
{
/**
  \class cow_ptr
  \brief Implements a copy on write data template
  \version 1.0
  \date February 2006
  \author S.Ansell
  
  This version works only on data that is created via new().
  It is thread safe and works in the Standard template 
  libraries (but appropriate functionals are needed for
  sorting etc.).

  Renamed from RefControl on the 11/12/2007, 
  as it was agreed that copy on write pointer better 
  described the functionality of this class.

  The underlying data can be accessed via the normal pointer
  semantics but call the access function if the data is required
  to be modified.

  Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  
  File change history is stored at: <https://github.com/mantidproject/mantid>

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

/**
  Constructor : creates new data() object
*/
template<typename DataType>
cow_ptr<DataType>::cow_ptr() :
  Data(new DataType())
{ }


/**
  Copy constructor : double references the data object
  @param A :: object to copy
*/
template<typename DataType>
cow_ptr<DataType>::cow_ptr(const cow_ptr<DataType>& A) :
  Data(A.Data)
{ }

/**
  Assignment operator : double references the data object
  maybe drops the old reference.
  @param A :: object to copy
  @return *this
*/
template<typename DataType>
cow_ptr<DataType>& cow_ptr<DataType>::operator=(const cow_ptr<DataType>& A)
{
  if (this!=&A)
  {
    Data=A.Data;
  }
  return *this;
}

/**
  Assignment operator : double references the data object
  maybe drops the old reference.
  @param A :: object to copy
  @return *this
*/
template<typename DataType>
cow_ptr<DataType>& cow_ptr<DataType>::operator=(const ptr_type& A)
{
  if (this->Data != A)
  {
    Data=A;
  }
  return *this;
}


/**
  Destructor : No work is required since Data is
  a shared_ptr.
*/
template<typename DataType>
cow_ptr<DataType>::~cow_ptr()
{}

/**
  Access function.
  If data is shared, creates a copy of Data so that it can be modified.

  @return new copy of *this, if required
*/
template<typename DataType>
DataType& cow_ptr<DataType>::access()
{
  // Use a double-check for sharing so that we only
  // enter the critical region if absolutely necessary
  if (!Data.unique())
  {
    PARALLEL_CRITICAL(cow_ptr_access)
    {
      // Check again because another thread may have taken copy
      // and dropped reference count since previous check
      if (!Data.unique())
      {
        ptr_type oldData=Data;
        Data.reset();
        Data=ptr_type(new DataType(*oldData));
      }
    }
  }

  return *Data;
}

} // NAMESPACE Kernel



/// typedef for the data storage used in Mantid matrix workspaces
typedef std::vector<double> MantidVec;

/// typedef for the pointer to data storage used in Mantid matrix workspaces
typedef Kernel::cow_ptr<MantidVec> MantidVecPtr;


} // NAMESPACE Mantid

#endif //MANTIDKERNEL_COW_PTR_H
