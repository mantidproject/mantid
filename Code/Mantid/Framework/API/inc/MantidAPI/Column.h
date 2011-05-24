#ifndef MANTID_API_ICOLUMN_H_
#define MANTID_API_ICOLUMN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/Logger.h"

#include <boost/shared_ptr.hpp>
#include <string>
#include <typeinfo>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace API
{
/** \class Column

    Column is the base class for columns of TableWorkspace.


    \author Roman Tolchenov
    \date 31/10/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
#ifdef _WIN32
#ifdef IN_MANTID_API
  #define Column_DllExport __declspec( dllexport )
#else
  #define Column_DllExport __declspec( dllimport )
#endif
#else
  #define Column_DllExport
  #define Column_DllImport
#endif

class Column_DllExport Column
{
public:
    /// Virtual destructor
    virtual ~Column() {}

    /// Name (caption) of the column.
    const std::string& name()const{return m_name;}

    /// Type of the column data.
    const std::string& type()const{return m_type;}

    /// Renames the column.
    void setName(const std::string& str){m_name = str;}

    /// Number of individual elements in the column.
    virtual size_t size()const = 0;

    /// Returns typeid for the data in the column
    virtual const std::type_info& get_type_info()const = 0;

    /// Returns typeid for the pointer type to the data element in the column
    virtual const std::type_info& get_pointer_type_info()const = 0;

    /// Prints
    virtual void print(std::ostream& s, size_t index) const = 0;

    /// Specialized type check
    virtual bool isBool()const = 0;

    /// Must return overall memory size taken by the column.
    virtual long int sizeOfData()const = 0;

    /// Templated method for returning a value. No type checks are done.
    template<class T>
    T& cell(size_t index)
    {
        return *static_cast<T*>(void_pointer(index));
    }

    /// Templated method for returning a value (const version). No type checks are done.
    template<class T>
    const T& cell(size_t index)const
    {
        return *static_cast<T*>(void_pointer(index));
    }

    /// Type check.
    template<class T>
    bool isType()const
    {
        return get_type_info() == typeid(T);
    }

protected:
    /// Sets the new column size.
    virtual void resize(size_t count) = 0;
    /// Inserts an item.
    virtual void insert(size_t index) = 0;
    /// Removes an item.
    virtual void remove(size_t index) = 0;
    /// Pointer to a data element
    virtual void* void_pointer(size_t index) = 0;

    std::string m_name;///< name
    std::string m_type;///< type
    friend class ColumnFactoryImpl;
    friend class ITableWorkspace;
    template<class T> friend class ColumnVector;
    /// Logger
    static Kernel::Logger& g_log;
};

/**  @class Boolean
    As TableColumn stores its data in a std::vector bool type cannot be used 
    in the same way as the other types. Class Boolean is used instead.
*/
struct Column_DllExport Boolean
{
    /// Default constructor
    Boolean():value(false){}
    /// Conversion from bool
    Boolean(bool b):value(b){}
    /// Returns bool
    operator bool(){return value;}
	/// equal to operator
	bool operator==(const Boolean& b)const
	{return(this->value==b.value);		
	}
    bool value;///< boolean value
};

/// Printing Boolean to an output stream
Column_DllExport std::ostream& operator<<(std::ostream& ,const API::Boolean& );

typedef boost::shared_ptr<Column> Column_sptr;

} // namespace API
} // Namespace Mantid
#endif /*MANTID_API_ICOLUMN_H_*/
