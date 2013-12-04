#ifndef MANTID_DATAOBJECTS_VECTORCOLUMN_H_
#define MANTID_DATAOBJECTS_VECTORCOLUMN_H_

#include "MantidAPI/Column.h"
#include "MantidKernel/System.h"


namespace Mantid
{
namespace DataObjects
{

  using namespace Kernel;
  using namespace API;

  /** VectorColumn : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
   
  template <class Type>
  class DLLExport VectorColumn : public Column
  {
  public:
    VectorColumn() 
    {
      m_type = typeName();
    }

    virtual ~VectorColumn()
    {}
    
    /// Number of individual elements in the column
    virtual size_t size() const
    { 
      return m_data.size();
    }

    /// Returns typeid for the data in the column
    virtual const std::type_info& get_type_info() const
    {
      return typeid( std::vector<Type> );
    }

    /// Returns typeid for the pointer type to the data element in the column
    virtual const std::type_info& get_pointer_type_info() const
    {
      return typeid( std::vector<Type>* );
    }

    /// Print specified item to the stream
    virtual void print(size_t index, std::ostream& s) const
    {
      // TODO: implement
    }

    /// Set item from a string value
    virtual void read(size_t index, const std::string & text)
    {
      // TODO: implement
    }

    /// Specialized type check
    virtual bool isBool() const
    { 
      return false;
    }

    /// Overall memory size taken by the column
    virtual long int sizeOfData() const
    {
      // TODO: implement
    }

    /// Create another copy of the column 
    virtual VectorColumn* clone() const
    {
      // TODO: implement
    }

    /// Cast to double
    virtual double toDouble(size_t i) const
    {
      throw std::runtime_error("VectorColumn is not convertible to double.");
    }

    /// Assign from double
    virtual void fromDouble(size_t i, double value)
    {
      throw std::runtime_error("VectorColumn is not assignable from double.");
    }

  protected:
    /// Sets the new column size.
    virtual void resize(size_t count)
    {
      // TODO: implement
    }

    /// Inserts an item.
    virtual void insert(size_t index)
    {
      // TODO: implement
    }

    /// Removes an item.
    virtual void remove(size_t index)
    {
    }

    /// Pointer to a data element
    virtual void* void_pointer(size_t index)
    {
      // TODO: implement
    }

    /// Pointer to a data element
    virtual const void* void_pointer(size_t index) const
    {
      // TODO: implement
    }

  private:
    /// All the vectors stored
    std::vector< std::vector<Type> > m_data;

    /// Returns the name of the column with the given type. Is specialized using DECLARE_VECTORCOLUMN
    std::string typeName();
  };


} // namespace DataObjects
} // namespace Mantid

#define DECLARE_VECTORCOLUMN(Type,TypeName) \
  template<> std::string VectorColumn<Type>::typeName() { return #TypeName; };

#endif  /* MANTID_DATAOBJECTS_VECTORCOLUMN_H_ */
