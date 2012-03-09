#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidPythonInterface/kernel/Registry/RegisterSingleValueHandler.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/converter/builtin_converters.hpp>
#include <vector>

using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::Workspace;
using Mantid::Kernel::DataItem_sptr;
using namespace boost::python;

namespace
{
  namespace bpl = boost::python;

  /**
   * Access a cell and return a corresponding Python type
   * @param self A reference to the TableWorkspace python object that we were called on
   * @param value A python object containing either a row index or a column name
   * @param row_or_col An integer giving the row if value is a string or the column if value is an index
   */
  PyObject * cell(ITableWorkspace &self, bpl::object value, int row_or_col)
  {
    // Find the column and row
    Mantid::API::Column_const_sptr column;
    int row(-1);
    if( PyString_Check(value.ptr()) )
    {
      column = self.getColumn( extract<std::string>(value)());
      row = row_or_col;
    }
    else
    {
      row = extract<int>(value)();
      column = self.getColumn(row_or_col);
    }
    // Boost.Python doesn't have a searchable registry for builtin types so
    // this is yet another lookup. There must be a better way to do this!

    const std::type_info & typeID = column->get_type_info();
    PyObject *result(NULL);
    if( typeID == typeid(double) )
    {
      result = to_python_value<const double&>()(column->cell<double>(row));
    }
    else if( typeID == typeid(std::string) )
    {
      result =  to_python_value<const std::string&>()(column->cell<std::string>(row));
    }
    else if( typeID == typeid(int) )
    {
      result =  to_python_value<const int &>()(column->cell<int>(row));
    }
    else if( typeID == typeid(int64_t) )
    {
      result =  to_python_value<const int64_t &>()(column->cell<int64_t>(row));
    }
    else if( typeID == typeid(bool) )
    {
      result =  to_python_value<const bool &>()(column->cell<bool>(row));
    }
    else if( typeID == typeid(float) )
    {
      result =  to_python_value<const float &>()(column->cell<float>(row));
    }
    else if( typeID == typeid(Mantid::Kernel::V3D) )
    {
      const converter::registration *entry = converter::registry::query(typeid(Mantid::Kernel::V3D));
      if(!entry) throw std::invalid_argument("ITableWorkspace::cell - Cannot find convert to V3D type.");
      result = entry->to_python((const void *)&column->cell<Mantid::Kernel::V3D>(row));
    }
    else
    {
      throw std::invalid_argument("ITableWorkspace::cell - Cannot convert column data type to python: " + column->type());
    }

    return result;
  }

  /**
   * Access a cell and return a corresponding Python type
   * @param self A reference to the TableWorkspace python object that we were called on
   * @param value A python object containing a column name or index
   */
  bpl::list column(ITableWorkspace &self, bpl::object value)
  {
    // Find the column and row
    Mantid::API::Column_const_sptr column;
    if( PyString_Check(value.ptr()) )
    {
      column = self.getColumn( extract<std::string>(value)());
    }
    else
    {
      column = self.getColumn(extract<int>(value)());
    }

    // Boost.Python doesn't have a searchable registry for builtin types so
    // this is yet another lookup. There must be a better way to do this!
    const std::type_info & typeID = column->get_type_info();
    const std::size_t numRows = column->size();

    bpl::list result;
    if( typeID == typeid(double) )
    {
      for (size_t i = 0; i < numRows; i++)
      {
        result.append(column->cell<double>(i));
      }
    }
    else if( typeID == typeid(std::string) )
    {
      for (size_t i = 0; i < numRows; i++)
      {
        result.append(column->cell<std::string>(i));
      }
    }
    else if( typeID == typeid(int) )
    {
      for (size_t i = 0; i < numRows; i++)
      {
        result.append(column->cell<int>(i));
      }
    }
    else if( typeID == typeid(int64_t) )
    {
      for (size_t i = 0; i < numRows; i++)
      {
        result.append(column->cell<int64_t>(i));
      }
    }
    else if( typeID == typeid(bool) )
    {
      for (size_t i = 0; i < numRows; i++)
      {
        result.append(column->cell<bool>(i));
      }
    }
    else if( typeID == typeid(float) )
    {
      for (size_t i = 0; i < numRows; i++)
      {
        result.append(column->cell<float>(i));
      }
    }
    else if( typeID == typeid(Mantid::Kernel::V3D) )
    {
      const converter::registration *entry = converter::registry::query(typeid(Mantid::Kernel::V3D));
      if(!entry) throw std::invalid_argument("ITableWorkspace::cell - Cannot find convert to V3D type.");
      for (size_t i = 0; i < numRows; i++)
      {
        result.append(column->cell<Mantid::Kernel::V3D>(i));
      }
    }
    else
    {
      throw std::invalid_argument("ITableWorkspace::cell - Cannot convert column data type to python: " + column->type());
    }

    return result;
  }
  /**
   * Access a cell and return a corresponding Python type
   * @param self A reference to the TableWorkspace python object that we were called on
   * @param row An integer giving the row
   */
  PyObject * row(ITableWorkspace &self, int row)
  {
    if (row < 0)
        throw std::invalid_argument("Cannot specify negative row number");
    if (row >= static_cast<int>(self.rowCount()))
        throw std::invalid_argument("Cannot specify row larger than number of rows");

    int numCols = static_cast<int>(self.columnCount());

    PyObject *result = PyDict_New();

    for (int col = 0; col < numCols; col++)
    {
        Mantid::API::Column_const_sptr column = self.getColumn(col);

        // Boost.Python doesn't have a searchable registry for builtin types so
        // this is yet another lookup. There must be a better way to do this!

        const std::type_info & typeID = column->get_type_info();
        PyObject *value(NULL);
        if( typeID == typeid(double) )
        {
          value = to_python_value<const double&>()(column->cell<double>(row));
        }
        else if( typeID == typeid(std::string) )
        {
          value =  to_python_value<const std::string&>()(column->cell<std::string>(row));
        }
        else if( typeID == typeid(int) )
        {
          value =  to_python_value<const int &>()(column->cell<int>(row));
        }
        else if( typeID == typeid(int64_t) )
        {
          value =  to_python_value<const int64_t &>()(column->cell<int64_t>(row));
        }
        else if( typeID == typeid(bool) )
        {
          value =  to_python_value<const bool &>()(column->cell<bool>(row));
        }
        else if( typeID == typeid(float) )
        {
          value =  to_python_value<const float &>()(column->cell<float>(row));
        }
        else if( typeID == typeid(Mantid::Kernel::V3D) )
        {
          const converter::registration *entry = converter::registry::query(typeid(Mantid::Kernel::V3D));
          if(!entry) throw std::invalid_argument("ITableWorkspace::cell - Cannot find convert to V3D type.");
          value = entry->to_python((const void *)&column->cell<Mantid::Kernel::V3D>(row));
        }
        else
        {
          throw std::invalid_argument("ITableWorkspace::cell - Cannot convert column data type to python: " + column->type());
        }

        if (PyDict_SetItemString(result, column->name().c_str(), value))
            throw std::runtime_error("Error while building dict");
    }

    return result;
  }
}

void export_ITableWorkspace()
{
  register_ptr_to_python<ITableWorkspace_sptr>();

  class_<ITableWorkspace,bases<Workspace>, boost::noncopyable>("ITableWorkspace", no_init)
    .def("columnCount", &ITableWorkspace::columnCount, "Returns the number of columns in the workspace")
    .def("rowCount", &ITableWorkspace::rowCount, "Returns the number of rows within the workspace")
    .def("__len__",  &ITableWorkspace::rowCount, "Returns the number of rows within the workspace")
    .def("getColumnNames",&ITableWorkspace::getColumnNames, "Return a list of the column names")
    .def("keys",          &ITableWorkspace::getColumnNames, "Return a list of the column names")
    .def("column", &column, "Return all values of a specific column as a list")
    .def("row", &row, "Return all values of a specific row as a dict")
    .def("cell", &cell, "Return the given cell. If the first argument is a number then it is interpreted as a row"
         "otherwise it is interpreted as a column name")
  ;

  REGISTER_SINGLEVALUE_HANDLER(ITableWorkspace_sptr);
}

