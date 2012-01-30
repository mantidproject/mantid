#ifndef MANTID_DATAOBJECTS_TABLECOLUMN_H_
#define MANTID_DATAOBJECTS_TABLECOLUMN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAPI/Column.h"
#include "MantidKernel/Logger.h"

#include <vector>
#include <typeinfo>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <limits>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------

namespace DataObjects
{

template< class T>
class TableVector;

/** \class TableColumn

    Class TableColumn implements abstract class Column for any copyable data type.
    A TableColumn is created using TableWorkspace::addColumn(type,name).
    type is the simbolic name of the data type which must be first declared with
    DECLARE_TABLECOLUMN macro. Predeclared types are:

    "int"    for int
    "float"  for float
    "double" for double
    "bool"   for Boolean
    "str"    for std::string
    "V3D"    for Mantid::Kernel::V3D

    Boolean is used instead of bool because of bool's non-standard treatmemt in std::vector.

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
template <class Type>
class DLLExport TableColumn: public API::Column
{
  /// Helper struct helping to write a generic casting to double
  struct InconvertibleToDoubleType
  {
    /// Constructor
    InconvertibleToDoubleType(const Type&){}
    /// Constructor
    InconvertibleToDoubleType(const double&){}
    /// Convertion to double throws a runtime_error.
    operator double() const {throw std::runtime_error(std::string("Cannot convert ")+typeid(Type).name()+" to double.");}
    operator Type() const {throw std::runtime_error(std::string("Cannot convert double to ")+typeid(Type).name()+".");}
  };
public:
    TableColumn(){
        int length = sizeof(Type);
        std::string name = std::string(typeid(Type).name());
        if((name.find("i")!=std::string::npos)||(name.find("l")!=std::string::npos)||
            (name.find("x")!=std::string::npos)){
            if(length==4){                
                this->m_type=="int";
            }
            if(length==8){
                this->m_type=="int64";
            }
        }
        if(name.find("f")!=std::string::npos){
            this->m_type ="float";
        }
        if(name.find("d")!=std::string::npos){
            this->m_type ="double";
        }
        if(this->m_type.empty()){
            this->m_type=name;
        }
    }

    //TableColumn();
    /// Virtual destructor.
    virtual ~TableColumn(){}
    /// Number of individual elements in the column.
    size_t size()const{return m_data.size();}
    /// Type id of the data in the column
    const std::type_info& get_type_info()const{return typeid(Type);}
    /// Type id of the pointer to data in the column
    const std::type_info& get_pointer_type_info()const{return typeid(Type*);}
    /// Output to an ostream.
    void print(size_t index, std::ostream& s)const{s << m_data[index];}
    /// Read in a string and set the value at the given index
    void read(size_t index, const std::string & text);
    /// Type check
    bool isBool()const{return typeid(Type) == typeid(API::Boolean);}
    /// Memory used by the column
    long int sizeOfData()const{return static_cast<long int>(m_data.size()*sizeof(Type));}
    /// Clone
    virtual TableColumn* clone() const
    {
      TableColumn* temp = new TableColumn();
      temp->m_data = this->m_data;
      temp->setName(this->m_name);
      return temp;
    }

    /** 
     * Cast an element to double if possible. If it's impossible boost::numeric::bad_numeric_cast
     * is throw. In case of an overflow boost::numeric::positive_overflow or boost::numeric::negative_overflow
     * is throw.
     * @param i :: The index to an element.
     */
    virtual double toDouble(size_t i)const
    {
      typedef boost::mpl::if_c< 
        boost::is_convertible<double, Type>::value, Type, InconvertibleToDoubleType>::type DoubleType;
      return boost::numeric_cast<double,DoubleType>(m_data[i]);
    }

    /** 
     * Cast an element to double if possible. If it's impossible boost::numeric::bad_numeric_cast
     * is throw. In case of an overflow boost::numeric::positive_overflow or boost::numeric::negative_overflow
     * is throw.
     * @param i :: The index to an element.
     */
    virtual void fromDouble(size_t i, double value)
    {
      typedef boost::mpl::if_c< 
        boost::is_convertible<double, Type>::value, Type, InconvertibleToDoubleType>::type DoubleType;
      m_data[i] = boost::numeric_cast<DoubleType,double>(value);
    }

    /// Reference to the data.
    std::vector<Type>& data(){return m_data;}

    /// return a value casted to double; the users responsibility is to be sure, that the casting is possible
    double operator[](size_t i)const
    {
        try{   
            return boost::lexical_cast<double>(m_data[i]);
        }catch(...){
            return std::numeric_limits<double>::quiet_NaN();    
        }
 
    }

protected:
    /// Resize.
    void resize(size_t count){m_data.resize(count);}
    /// Inserts default value at position index. 
    void insert(size_t index)
    {
        if (index < m_data.size())
            m_data.insert(m_data.begin()+index,Type()); 
        else
            m_data.push_back(Type());
    }
    /// Removes an item at index.
    void remove(size_t index){m_data.erase(m_data.begin()+index);}
    /// Returns a pointer to the data element.
    void* void_pointer(size_t index){return &m_data[index];}
    /// Returns a pointer to the data element.
    const void* void_pointer(size_t index)const {return &m_data[index];}
private:
    /// Column data
    std::vector<Type> m_data;
    friend class TableWorkspace;  
};

/// Read in a string and set the value at the given index
template<typename Type>
void TableColumn<Type>::read(size_t index, const std::string & text)
{
  std::istringstream istr(text);
  istr >> m_data[index];
}

template<>
double TableColumn<API::Boolean>::toDouble(size_t i)const
{
  return m_data[i] ? 1.0 : 0.0;
}

template<>
void TableColumn<API::Boolean>::fromDouble(size_t i, double value)
{
  m_data[i] = value != 0.0;
}

/// Shared pointer to a column with aoutomatic type cast and data type check.
/// Can be created with TableWorkspace::getColumn(...)
template< class T>
class TableColumn_ptr: public boost::shared_ptr<TableColumn<T> >
{
public:
    /** Constructor
        @param c :: Shared pointer to a column
      */
    TableColumn_ptr(boost::shared_ptr<API::Column> c):boost::shared_ptr<TableColumn<T> >(boost::dynamic_pointer_cast<TableColumn<T> >(c))
    {
        if (this->get() == NULL)
        {
            Kernel::Logger& log = Kernel::Logger::get("TableWorkspace");
            std::string str = "Data type of column "+c->name()+" does not match "+typeid(T).name();
            log.error(str);
            throw std::runtime_error(str);
        }
    }
};

/// Special case of bool
template<>
class TableColumn_ptr<bool>: public TableColumn_ptr<API::Boolean>
{
public:
    /** Constructor
        @param c :: Shared pointer to a column
      */
    TableColumn_ptr(boost::shared_ptr<API::Column> c):TableColumn_ptr<API::Boolean>(c)
    {
        if (this->get() == NULL)
        {
            Kernel::Logger& log = Kernel::Logger::get("TableWorkspace");
            std::string str = "Data type of column "+c->name()+" does not match "+typeid(API::Boolean).name();
            log.error(str);
            throw std::runtime_error(str);
        }
    }
};

} // namespace DataObjects
} // Namespace Mantid

/*
    Macro to declare a type to be used with TableColumn.
    DataType is the actual C++ type. TypeName is a symbolic name, used in TableWorkspace::createColumn(...)
    TypeName can contain only letters, numbers and _s.
*/
#define DECLARE_TABLECOLUMN(DataType,TypeName) \
    namespace{ \
    Mantid::Kernel::RegistrationHelper register_column_##TypeName(  \
    (Mantid::API::ColumnFactory::Instance().subscribe< Mantid::DataObjects::TableColumn< DataType > >(#TypeName),0)); \
    } 

#endif /*MANTID_DATAOBJECTS_TABLECOLUMN_H_*/
