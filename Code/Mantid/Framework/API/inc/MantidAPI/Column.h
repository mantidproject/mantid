#ifndef MANTID_API_ICOLUMN_H_
#define MANTID_API_ICOLUMN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"

#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif
#include <cstring>
#include <string>
#include <limits>
#include <typeinfo>
#include <vector>

namespace Mantid
{

namespace API
{
/** \class Column

    Column is the base class for columns of TableWorkspace.


    \author Roman Tolchenov
    \date 31/10/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL Column
{
public:
    Column() : m_type("int"), m_plotType(-1000), m_isReadOnly(true) {};

    /// Virtual destructor
    virtual ~Column() {}

    /// Name (caption) of the column.
    const std::string& name()const{return m_name;}

    /// Type of the column data.
    const std::string& type()const{return m_type;}
    /// return value casted to double (should be pure virtual)
    virtual double operator[](size_t i)const{UNUSED_ARG(i); return std::numeric_limits<double>::quiet_NaN();}
    /// Renames the column.
    void setName(const std::string& str){m_name = str;}

    /// Number of individual elements in the column.
    virtual size_t size()const = 0;

    /// Returns typeid for the data in the column
    virtual const std::type_info& get_type_info()const = 0;

    /// Returns typeid for the pointer type to the data element in the column
    virtual const std::type_info& get_pointer_type_info()const = 0;

    /// Returns column read-only flag
    virtual bool getReadOnly() const
    {
      return m_isReadOnly;
    }

    /// Sets column read-only flag
    void setReadOnly(bool isReadOnly)
    {
      m_isReadOnly = isReadOnly;
    }

    /// Prints out the value to a stream
    virtual void print(size_t index, std::ostream& s) const = 0;

    /// Read in a string and set the value at the given index
    virtual void read(size_t index, const std::string & text )
    { UNUSED_ARG(text);UNUSED_ARG(index); }

    /// Specialized type check
    virtual bool isBool()const = 0;

    /// Must return overall memory size taken by the column.
    virtual long int sizeOfData()const = 0;

    /// Virtual constructor. Fully clone any column.
    virtual Column* clone() const = 0;

    /// Cast an element to double if possible
    virtual double toDouble(size_t index)const = 0;

    /// Assign an element from double if possible
    virtual void fromDouble(size_t index, double value) = 0;

    /// Sort all or part of a vector of indices according to values in corresponding cells of this column.
    /// Fill in a vector of ranges of equal values. A range is a [begin,end) pair of indices in indexVec.
    /// @param ascending :: Sort in ascending (true) or descending (false) order.
    /// @param start :: Starting index in indexVec to be sorted.
    /// @param end :: Ending index (one past last) in indexVec to be sorted.
    /// @param indexVec :: A vector of indices. On input it must contain all indices from 0 to this->size()-1. 
    ///          On output indices between start and end are sorted in ascending order of values in this column.
    ///          So that: cell(indexVec[i]) <= cell(indexVec[i+1])
    /// @param equalRanges :: Output only vector. For each pair p in equalRanges 
    ///          cell(indexVec[p.first]) == cell(indexVec[p.first+1]) == ... == cell(indexVec[p.end-1]).
    ///          If equalRanges is empty then there are no equal velues in this column.
    virtual void sortIndex( bool ascending, size_t start, size_t end, std::vector<size_t>& indexVec, std::vector<std::pair<size_t,size_t>>& equalRanges ) const;

    /// Re-arrange values in this column according to indices in indexVec
    virtual void sortValues( const std::vector<size_t>& indexVec );

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
        return *static_cast<const T*>(void_pointer(index));
    }

    /// Type check.
    template<class T>
    bool isType()const
    {
      return !std::strcmp(get_type_info().name(), typeid(T).name());
    }

    /// get plot type
    /// @return See description of setPlotType() for the interpretation of the returned int
    int getPlotType() const
    {
       return m_plotType;
    }

    /// Set plot type where
    void setPlotType(int t);

    /**
     * Fills a std vector with values from the column if the types are compatible.
     * @param vec :: The vector to fill in.
     */
    template<class T>
    void numeric_fill(std::vector<T>& vec) const
    {
      vec.resize(size());
      for(size_t i = 0; i < vec.size(); ++i)
      {
        vec[i] = static_cast<T>(toDouble(i));
      }
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
    /// Pointer to a data element
    virtual const void* void_pointer(size_t index) const = 0;

    std::string m_name;///< name
    std::string m_type;///< type

    /// plot type where
    /// None = 0 (means it has specifically been set to 'no plot type')
    /// NotSet = -1000 (this is the default and means plot style has not been set)
    /// X = 1, Y = 2, Z = 3, xErr = 4, yErr = 5, Label = 6
    int m_plotType;

    /// Column read-only flag
    bool m_isReadOnly;

    friend class ColumnFactoryImpl;
    friend class ITableWorkspace;
    template<class T> friend class ColumnVector;
};

/**  @class Boolean
    As TableColumn stores its data in a std::vector bool type cannot be used
    in the same way as the other types. Class Boolean is used instead.
*/
struct MANTID_API_DLL Boolean
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
    //
    operator double(void)const{return double(this->value);}
    bool value;///< boolean value
};

/// Printing Boolean to an output stream
MANTID_API_DLL std::ostream& operator<<(std::ostream& ,const API::Boolean& );
/// Redaing a Boolean from an input stream
MANTID_API_DLL std::istream& operator>>(std::istream& istr, API::Boolean&);

typedef boost::shared_ptr<Column> Column_sptr;
typedef boost::shared_ptr<const Column> Column_const_sptr;

} // namespace API
} // Namespace Mantid
#endif /*MANTID_API_ICOLUMN_H_*/
