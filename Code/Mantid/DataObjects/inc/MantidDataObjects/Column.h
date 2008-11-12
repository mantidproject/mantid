#ifndef MANTID_DATAOBJECTS_ICOLUMN_H_
#define MANTID_DATAOBJECTS_ICOLUMN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/System.h"

#include <string>
#include <typeinfo>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------

namespace DataObjects
{
    class TableWorkspace;
}

namespace DataObjects
{
/** \class IColumn

    IColumn is the base class for columns of TableWorkspace.


    \author Roman Tolchenov
    \date 31/10/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
class DLLExport Column
{
public:
    /// Name (caption) of the column.
    const std::string& name(){return m_name;}
    /// Type of the column data.
    const std::string& type(){return m_type;}
    /// Renames the column.
    void setName(const std::string& str){m_name = str;}
    /// Number of individual elements in the column.
    virtual int size() = 0;
protected:
    /// Sets the new column size.
    virtual void resize(int count) = 0;
    /// Inserts an item.
    virtual void insert(int index) = 0;
    /// Removes an item.
    virtual void remove(int index) = 0;
    /// Pointer to a data element
    virtual void* void_pointer(int index) = 0;
    virtual const std::type_info& get_type_info() = 0;
    virtual const std::type_info& get_pointer_type_info() = 0;
private:
    std::string m_name;///< name
    std::string m_type;///< type
    friend class ColumnFactoryImpl;
    friend class TableWorkspace;
};

} // namespace DataObjects
} // Namespace Mantid
#endif /*MANTID_DATAOBJECTS_ICOLUMN_H_*/
