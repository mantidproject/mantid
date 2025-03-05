// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/RegisterWorkspacePtrToPython.h"
#include "MantidPythonInterface/core/Converters/CloneToNDArray.h"
#include "MantidPythonInterface/core/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/NDArray.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"
#include "MantidPythonInterface/core/VersionCompat.h"

#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/python/class.hpp>
#include <boost/python/converter/builtin_converters.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/overloads.hpp>

#include <cstring>
#include <vector>

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using namespace Mantid::API;
using Mantid::PythonInterface::NDArray;
using Mantid::PythonInterface::Registry::RegisterWorkspacePtrToPython;
namespace Policies = Mantid::PythonInterface::Policies;
namespace Converters = Mantid::PythonInterface::Converters;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ITableWorkspace)

namespace {

// Numpy PyArray_IsIntegerScalar is broken for Python 3 for numpy < 1.11
#define TO_LONG PyLong_AsLong
#define STR_CHECK PyUnicode_Check
#if NPY_API_VERSION < 0x0000000a //(1.11)
#define IS_ARRAY_INTEGER(obj) (PyLong_Check(obj) || PyArray_IsScalar((obj), Integer))
#else
#define IS_ARRAY_INTEGER PyArray_IsIntegerScalar
#endif

/// Boost macro for "looping" over builtin types
#define BUILTIN_TYPES BOOST_PP_TUPLE_TO_LIST(8, (double, std::string, int, size_t, uint32_t, int64_t, float, uint64_t))
#define USER_TYPES BOOST_PP_TUPLE_TO_LIST(1, (Mantid::Kernel::V3D))
#define ARRAY_TYPES BOOST_PP_TUPLE_TO_LIST(2, (std::vector<int>, std::vector<double>))

/**
 * Get out the Python value from a specific cell of the supplied column. This is
 * meant to
 * reduce the amount of copy and pasted code in this file.
 * @param column The column to grab the value from.
 * @param typeID The python identifier of the column type.
 * @param row The row to get the value from.
 */
PyObject *getValue(const Mantid::API::Column_const_sptr &column, const std::type_info &typeID, const int row) {
  if (typeID.hash_code() == typeid(Mantid::API::Boolean).hash_code()) {
    bool res = column->cell<Mantid::API::Boolean>(row);
    return to_python_value<const bool &>()(res);
  }

#define GET_BUILTIN(R, _, T)                                                                                           \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                                                              \
    result = to_python_value<const T &>()(column->cell<T>(row));                                                       \
  }
#define GET_USER(R, _, T)                                                                                              \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                                                              \
    const converter::registration *entry = converter::registry::query(typeid(T));                                      \
    if (!entry)                                                                                                        \
      throw std::invalid_argument("Cannot find converter from C++ type.");                                             \
    result = entry->to_python((const void *)&column->cell<T>(row));                                                    \
  }
#define GET_ARRAY(R, _, T)                                                                                             \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                                                              \
    result = Converters::Clone::apply<T::value_type>::create1D(column->cell<T>(row));                                  \
  }

  // -- Use the boost preprocessor to generate a list of else if clause to cut
  // out copy and pasted code.
  PyObject *result(nullptr);
  if (false) {
  } // So that it always falls through to the list checking
  BOOST_PP_LIST_FOR_EACH(GET_BUILTIN, _, BUILTIN_TYPES)
  BOOST_PP_LIST_FOR_EACH(GET_ARRAY, _, ARRAY_TYPES)
  BOOST_PP_LIST_FOR_EACH(GET_USER, _, USER_TYPES)
  else {
    throw std::invalid_argument("Cannot convert C++ type to Python: " + column->type());
  }
  return result;
}

/**
 * Sets a value in a particular column and row from a python object
 * @param column :: A pointer to the column object
 * @param row :: The index of the row
 * @param value :: The value to set
 */
void setValue(const Column_sptr &column, const int row, const object &value) {
  const auto &typeID = column->get_type_info();

  // Special case: Treat Mantid Boolean as normal bool
  if (typeID.hash_code() == typeid(Mantid::API::Boolean).hash_code()) {
    column->cell<Mantid::API::Boolean>(row) = extract<bool>(value)();
    return;
  }

  // Special case: Boost has issues with NumPy ints, so use Python API instead
  // to check this first
  if (typeID.hash_code() == typeid(int).hash_code() && IS_ARRAY_INTEGER(value.ptr())) {
    column->cell<int>(row) = static_cast<int>(TO_LONG(value.ptr()));
    return;
  }

// Macros for all other types
#define SET_CELL(R, _, T)                                                                                              \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                                                              \
    column->cell<T>(row) = extract<T>(value)();                                                                        \
  }
#define SET_VECTOR_CELL(R, _, T)                                                                                       \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                                                              \
    if (!NDArray::check(value)) {                                                                                      \
      column->cell<T>(row) = Converters::PySequenceToVector<T::value_type>(value)();                                   \
    } else {                                                                                                           \
      column->cell<T>(row) = Converters::NDArrayToVector<T::value_type>(value)();                                      \
    }                                                                                                                  \
  }

  // -- Use the boost preprocessor to generate a list of else if clause to cut
  // out copy and pasted code.
  if (false) {
  } // So that it always falls through to the list checking
  BOOST_PP_LIST_FOR_EACH(SET_CELL, _, BUILTIN_TYPES)
  BOOST_PP_LIST_FOR_EACH(SET_CELL, _, USER_TYPES)
  BOOST_PP_LIST_FOR_EACH(SET_VECTOR_CELL, _, ARRAY_TYPES)
  else {
    throw std::invalid_argument("Cannot convert Python type to C++: " + column->type());
  }
}

/**
 * Add a column with a given plot type to the TableWorkspace
 * @param self Reference to TableWorkspace this is called on
 * @param type The data type of the column to add
 * @param name The name of the column to add
 * @param plottype The plot type to set on the column
 * @return Boolean true if successful, false otherwise
 */
bool addColumnPlotType(ITableWorkspace &self, const std::string &type, const std::string &name, int plottype) {
  auto newColumn = self.addColumn(type, name);

  if (newColumn)
    newColumn->setPlotType(plottype);

  return newColumn != nullptr;
}

/**
 * Add a column to the TableWorkspace (with default plot type)
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @param type The data type of the column to add
 * @param name The name of the column to add
 * @return A boolean indicating success or failure. Note that this is different
 * to the corresponding C++ method, which returns a pointer to the
 * newly-created column (as the Column class is not exposed to python).
 */
bool addColumnSimple(ITableWorkspace &self, const std::string &type, const std::string &name) {
  return self.addColumn(type, name) != nullptr;
}

/**
 * Add a column to the TableWorkspace that cannot be edited.
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @param type The data type of the column to add
 * @param name The name of the column to add
 * @return A boolean indicating success or failure. Note that this is different
 * to the corresponding C++ method, which returns a pointer to the
 * newly-created column (as the Column class is not exposed to python).
 */
bool addReadOnlyColumn(ITableWorkspace &self, const std::string &type, const std::string &name) {
  auto newColumn = self.addColumn(type, name);
  newColumn->setReadOnly(true);
  return true;
}

/**
 * Get the plot type of a column given by name or index
 * @param self Reference to TableWorkspace this is called on
 * @param column Name or index of column
 * @return PlotType: 0=None, 1=X, 2=Y, 3=Z, 4=xErr, 5=yErr, 6=Label
 */
int getPlotType(ITableWorkspace &self, const object &column) {
  // Find the column
  Mantid::API::Column_const_sptr colptr;
  if (STR_CHECK(column.ptr())) {
    colptr = self.getColumn(extract<std::string>(column)());
  } else {
    colptr = self.getColumn(extract<int>(column)());
  }

  return colptr->getPlotType();
}

/**
 * Set the plot type of a column given by name or index
 * @param self Reference to TableWorkspace this is called on
 * @param column Name or index of column
 * @param ptype PlotType: 0=None, 1=X, 2=Y, 3=Z, 4=xErr, 5=yErr, 6=Label
 * @param linkedCol Index of the column that the column parameter is linked to
 * (typically used for an error column)
 */
void setPlotType(ITableWorkspace &self, const object &column, int ptype, int linkedCol = -1) {
  // Find the column
  Mantid::API::Column_sptr colptr;
  if (STR_CHECK(column.ptr())) {
    colptr = self.getColumn(extract<std::string>(column)());
  } else {
    colptr = self.getColumn(extract<int>(column)());
  }
  colptr->setPlotType(ptype);
  if (linkedCol >= 0) {
    colptr->setLinkedYCol(linkedCol);
  }
  self.modified();
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
// cppcheck-suppress unknownMacro
BOOST_PYTHON_FUNCTION_OVERLOADS(setPlotType_overloads, setPlotType, 3, 4)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

/**
 * Get the data column associated with a Y error column
 * @param self Reference to TableWorkspace this is called on
 * @param column Name or index of column
 * @return index of the associated Y column
 */
int getLinkedYCol(ITableWorkspace &self, const object &column) {
  // Find the column
  Mantid::API::Column_const_sptr colptr;
  if (STR_CHECK(column.ptr())) {
    colptr = self.getColumn(extract<std::string>(column)());
  } else {
    colptr = self.getColumn(extract<int>(column)());
  }

  return colptr->getLinkedYCol();
}

/**
 * Link a data column associated with a Y error column
 * @param self Reference to TableWorkspace this is called on
 * @param column Name or index of error column
 * @return index of the associated Y column
 */
void setLinkedYCol(ITableWorkspace &self, const object &errColumn, const int dataColomn) {
  // Find the column
  Mantid::API::Column_sptr colptr;
  if (STR_CHECK(errColumn.ptr())) {
    colptr = self.getColumn(extract<std::string>(errColumn)());
  } else {
    colptr = self.getColumn(extract<int>(errColumn)());
  }

  colptr->setLinkedYCol(dataColomn);
}

/**
 * Access a cell and return a corresponding Python type
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @param value A python object containing a column name or index
 */
PyObject *column(const ITableWorkspace &self, const object &value) {
  // Find the column and row
  Mantid::API::Column_const_sptr column;
  if (STR_CHECK(value.ptr())) {
    column = self.getColumn(extract<std::string>(value)());
  } else {
    column = self.getColumn(extract<int>(value)());
  }
  const std::type_info &typeID = column->get_type_info();
  const auto numRows = static_cast<int>(column->size());

  PyObject *result = PyList_New(numRows);
  for (int i = 0; i < numRows; i++) {
    if (PyList_SetItem(result, i, getValue(column, typeID, i)))
      throw std::runtime_error("Error while building list");
  }

  return result;
}

/**
 * Access a cell and return a corresponding Python type
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @param row An integer giving the row
 */
PyObject *row(ITableWorkspace &self, int row) {
  if (row < 0)
    throw std::invalid_argument("Cannot specify negative row number");
  if (row >= static_cast<int>(self.rowCount()))
    throw std::invalid_argument("Cannot specify row larger than number of rows");

  auto numCols = static_cast<int>(self.columnCount());

  PyObject *result = PyDict_New();

  for (int columnIndex = 0; columnIndex < numCols; columnIndex++) {
    Mantid::API::Column_const_sptr col = self.getColumn(columnIndex);
    const std::type_info &typeID = col->get_type_info();

    if (PyDict_SetItemString(result, col->name().c_str(), getValue(col, typeID, row)))
      throw std::runtime_error("Error while building dict");
  }

  return result;
}

/**
 * Return the C++ types for all columns
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 */
boost::python::list columnTypes(ITableWorkspace &self) {
  auto numCols = static_cast<int>(self.columnCount());

  boost::python::list types;

  for (int colIndex = 0; colIndex < numCols; colIndex++) {
    const auto col = self.getColumn(colIndex);
    const auto &type = col->type();
    types.append(type);
  }

  return types;
}

/**
 * Adds a new row in the table, where the items are given in a dictionary
 * object mapping {column name:value}. It must contain a key-value entry for
 * every column in the row, otherwise the insert will fail.
 *
 * @param self :: A reference to the ITableWorkspace object
 * @param rowItems :: A dictionary defining the items in the row
 */
void addRowFromDict(ITableWorkspace &self, const dict &rowItems) {
  // rowItems must contain an entry for every column
  auto nitems = boost::python::len(rowItems);
  if (nitems != static_cast<decltype(nitems)>(self.columnCount())) {
    throw std::invalid_argument("Number of values given does not match the number of columns. "
                                "Expected: " +
                                std::to_string(self.columnCount()));
  }

  // Add a new row to populate with values
  const auto rowIndex = static_cast<int>(self.rowCount());
  self.appendRow();

  // Declared in this scope so we can access them in catch block
  Column_sptr col; // Column in table
  object value;    // Value from dictionary

  try {
    // Retrieve and set the value for each column
    auto columns = self.getColumnNames();
    for (auto &iter : columns) {
      col = self.getColumn(iter);
      value = rowItems[iter];
      setValue(col, rowIndex, value);
    }
  } catch (error_already_set &) {
    // One of the columns wasn't found in the dictionary
    if (PyErr_ExceptionMatches(PyExc_KeyError)) {
      std::ostringstream msg;
      msg << "Missing key-value entry for column ";
      msg << "<" << col->name() << ">";
      PyErr_SetString(PyExc_KeyError, msg.str().c_str());
    }

    // Wrong type of data for one of the columns
    if (PyErr_ExceptionMatches(PyExc_TypeError)) {
      std::ostringstream msg;
      msg << "Wrong datatype for column <" << col->name() << "> ";
      msg << "(expected <" << col->type() << ">)";
      PyErr_SetString(PyExc_TypeError, msg.str().c_str());
    }

    // Remove the new row since populating it has failed
    self.removeRow(rowIndex);
    throw;
  }
  self.modified();
}

/**
 * Adds a new row in the table, where the items are given in an ordered python
 * sequence type (list, tuple, numpy array, etc). It must be the same length as
 * the number of columns or the insert will fail.
 *
 * @param self :: A reference to the ITableWorkspace object
 * @param rowItems :: A sequence containing the column values in the row
 */
void addRowFromSequence(ITableWorkspace &self, const object &rowItems) {
  // rowItems must contain an entry for every column
  auto nitems = boost::python::len(rowItems);
  if (nitems != static_cast<decltype(nitems)>(self.columnCount())) {
    throw std::invalid_argument("Number of values given does not match the number of columns. "
                                "Expected: " +
                                std::to_string(self.columnCount()));
  }

  // Add a new row to populate with values
  const auto rowIndex = static_cast<int>(self.rowCount());
  self.appendRow();

  // Loop over sequence and set each column value in same order
  for (decltype(nitems) i = 0; i < nitems; ++i) {
    auto col = self.getColumn(i);
    auto value = rowItems[i];

    try {
      setValue(col, rowIndex, value);
    } catch (error_already_set &) {
      // Wrong type of data for one of the columns
      if (PyErr_ExceptionMatches(PyExc_TypeError)) {
        std::ostringstream msg;
        msg << "Wrong datatype for column <" << col->name() << "> ";
        msg << "(expected <" << col->type() << ">)";
        PyErr_SetString(PyExc_TypeError, msg.str().c_str());
      }

      // Remove the new row since populating it has failed
      self.removeRow(rowIndex);
      throw;
    }
  }
  self.modified();
}

/**
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @param value A python object containing either a row index or a column name
 * @param row_or_col An integer giving the row if value is a string or the
 * column if value is an index
 * @param column [Out]:: The column pointer will be stored here
 * @param rowIndex [Out]:: The row index will be stored here
 */
void getCellLoc(ITableWorkspace &self, const object &col_or_row, const int row_or_col, Column_sptr &column,
                int &rowIndex) {
  if (STR_CHECK(col_or_row.ptr())) {
    column = self.getColumn(extract<std::string>(col_or_row)());
    rowIndex = row_or_col;
  } else {
    rowIndex = extract<int>(col_or_row)();
    column = self.getColumn(row_or_col);
  }
}

/**
 * Returns an appropriate Python object for the value at the given cell
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @param value A python object containing either a row index or a column name
 * @param row_or_col An integer giving the row if value is a string or the
 * column if value is an index
 */
PyObject *cell(ITableWorkspace &self, const object &value, int row_or_col) {
  // Find the column and row
  Mantid::API::Column_sptr col;
  int rowIndex;
  getCellLoc(self, value, row_or_col, col, rowIndex);
  const std::type_info &typeID = col->get_type_info();
  return getValue(col, typeID, rowIndex);
}

/**
 * Sets the value of the given cell
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @param value A python object containing either a row index or a column name
 * @param row_or_col An integer giving the row if value is a string or the
 * column if value is an index
 */
void setCell(ITableWorkspace &self, const object &col_or_row, const int row_or_col, const object &value,
             const bool &notify_replace) {
  Mantid::API::Column_sptr col;
  int rowIndex;
  getCellLoc(self, col_or_row, row_or_col, col, rowIndex);
  setValue(col, rowIndex, value);

  if (notify_replace) {
    self.modified();
  }
}

/**
 * Get whether or not a column given by name or index is read only.
 * @param self Reference to TableWorkspace this is called on
 * @param column Name or index of column
 * @return True if read only, False otherwise.
 */
bool isColumnReadOnly(ITableWorkspace &self, const object &column) {
  // Find the column
  Mantid::API::Column_const_sptr colptr;
  if (STR_CHECK(column.ptr())) {
    colptr = self.getColumn(extract<std::string>(column)());
  } else {
    colptr = self.getColumn(extract<int>(column)());
  }
  return colptr->getReadOnly();
}

/**
 * Set whether or not a column given by name or index should be read only.
 * @param self Reference to the TableWorkspace this was called on.
 * @param column Name or index of column.
 * @param readOnly True if read only, False otherwise.
 */
void setColumnReadOnly(ITableWorkspace &self, const object &column, const bool readOnly) {
  // Find the column
  Mantid::API::Column_sptr colptr;
  if (STR_CHECK(column.ptr())) {
    colptr = self.getColumn(extract<std::string>(column)());
  } else {
    colptr = self.getColumn(extract<int>(column)());
  }
  colptr->setReadOnly(readOnly);
  self.modified();
}
} // namespace

/**
 * Get the contents of the workspace as a python dictionary
 *
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @returns a boost python dictionary object with keys that are column names and
 * values which are lists of the column values.
 */
dict toDict(const ITableWorkspace &self) {
  dict result;

  for (const auto &name : self.getColumnNames()) {
    handle<> handle(column(self, object(name)));
    object values(handle);
    result[name] = values;
  }

  return result;
}

class ITableWorkspacePickleSuite : public boost::python::pickle_suite {
public:
  static dict getstate(const ITableWorkspace &ws) {
    dict data;
    data["data"] = toDict(ws);
    data["meta_data"] = writeMetaData(ws);
    return data;
  }

  static void setstate(ITableWorkspace &ws, const dict &state) {
    readMetaData(ws, state);
    readData(ws, state);
  }

private:
  /** Write the meta data from a table workspace to a python dict
   *
   * @param ws :: the workspace to load data into
   */
  static dict writeMetaData(const ITableWorkspace &ws) {
    list columnTypes;
    list columnNames;

    const auto &names = ws.getColumnNames();
    for (const auto &name : names) {
      const auto &column = ws.getColumn(name);
      columnNames.append(name);
      columnTypes.append(column->type());
    }

    dict metaData;
    metaData["column_names"] = columnNames;
    metaData["column_types"] = columnTypes;
    return metaData;
  }

  /** Read the meta data from a python dict into the table workspace
   *
   * This will read information relating to the column names and data
   * types to be stored in the new table
   *
   * @param ws :: the workspace to load data into
   * @param state :: the pickled state of the table
   */
  static void readMetaData(ITableWorkspace &ws, const dict &state) {
    const auto &metaData = state["meta_data"];
    const auto &columnNames = metaData["column_names"];
    const auto &columnTypes = metaData["column_types"];

    auto numColumns = len(columnNames);
    for (decltype(numColumns) colIndex = 0; colIndex < numColumns; ++colIndex) {
      const auto &key = columnNames[colIndex];
      const auto &value = columnTypes[colIndex];
      const auto &name = extract<std::string>(key);
      const auto &type = extract<std::string>(value);
      ws.addColumn(type, name);
    }
  }

  /** Read the data from a python dict into the table workspace
   *
   * @param ws :: the workspace to load data into
   * @param state :: the pickled state of the table
   */
  static void readData(ITableWorkspace &ws, const dict &state) {
    const auto &data = state["data"];
    const auto &names = ws.getColumnNames();

    if (names.empty()) {
      return;
    }

    auto numRows = len(data[names[0]]);
    for (int rowIndex = 0; rowIndex < numRows; ++rowIndex) {
      ws.appendRow();
      for (const auto &name : names) {
        setValue(ws.getColumn(name), static_cast<int>(rowIndex), data[name][rowIndex]);
      }
    }
  }
};

void export_ITableWorkspace() {
  using Mantid::PythonInterface::Policies::VectorToNumpy;

  std::string iTableWorkspace_docstring = "Most of the information from a table workspace is returned ";
  iTableWorkspace_docstring += "as native copies. All of the column accessors return lists while the ";
  iTableWorkspace_docstring += "rows return dicts. This object does support the idom 'for row in ";
  iTableWorkspace_docstring += "ITableWorkspace'.";

  class_<ITableWorkspace, bases<Workspace>, boost::noncopyable>("ITableWorkspace", iTableWorkspace_docstring.c_str(),
                                                                no_init)
      .def_pickle(ITableWorkspacePickleSuite())
      .def("addColumn", &addColumnSimple, (arg("self"), arg("type"), arg("name")),
           "Add a named column with the given type. Recognized types are: "
           "int,float,double,bool,str,V3D,long64")

      .def("addColumn", &addColumnPlotType, (arg("self"), arg("type"), arg("name"), arg("plottype")),
           "Add a named column with the given datatype "
           "(int,float,double,bool,str,V3D,long64) "
           "\nand plottype "
           "(0 = None, 1 = X, 2 = Y, 3 = Z, 4 = xErr, 5 = yErr, 6 = Label).")

      .def("addReadOnlyColumn", &addReadOnlyColumn, (arg("self"), arg("type"), arg("name")),
           "Add a read-only, named column with the given type. Recognized types are: "
           "int,float,double,bool,str,V3D,long64")

      .def("getPlotType", &getPlotType, (arg("self"), arg("column")),
           "Get the plot type of given column as an integer. "
           "Accepts column name or index. \nPossible return values: "
           "(0 = None, 1 = X, 2 = Y, 3 = Z, 4 = xErr, 5 = yErr, 6 = Label).")

      .def("setPlotType", setPlotType,
           setPlotType_overloads((arg("self"), arg("column"), arg("ptype"), arg("linkedCol") = -1),
                                 "Set the plot type of given column. "
                                 "Accepts column name or index. \nPossible type values: "
                                 "(0 = None, 1 = X, 2 = Y, 3 = Z, 4 = xErr, 5 = yErr, 6 = "
                                 "Label)."))

      .def("getLinkedYCol", &getLinkedYCol, (arg("self"), arg("column")),
           "Get the data column associated with a given error column. ")

      .def("setLinkedYCol", &setLinkedYCol, (arg("self"), arg("errColumn"), arg("dataColumn")),
           "Set the data column associated with a given error column. ")

      .def("removeColumn", &ITableWorkspace::removeColumn, (arg("self"), arg("name")), "Remove the named column.")

      .def("columnCount", &ITableWorkspace::columnCount, arg("self"), "Returns the number of columns in the workspace.")

      .def("rowCount", &ITableWorkspace::rowCount, arg("self"), "Returns the number of rows within the workspace.")

      .def("setRowCount", &ITableWorkspace::setRowCount, (arg("self"), arg("count")),
           "Resize the table to contain count rows.")

      .def("__len__", &ITableWorkspace::rowCount, arg("self"), "Returns the number of rows within the workspace.")

      .def("getColumnNames", &ITableWorkspace::getColumnNames, arg("self"),
           boost::python::return_value_policy<VectorToNumpy>(), "Return a list of the column names.")

      .def("keys", &ITableWorkspace::getColumnNames, arg("self"), boost::python::return_value_policy<VectorToNumpy>(),
           "Return a list of the column names.")

      .def("column", &column, (arg("self"), arg("column")), "Return all values of a specific column as a list.")

      .def("row", &row, (arg("self"), arg("row")), "Return all values of a specific row as a dict.")

      .def("columnTypes", &columnTypes, arg("self"), "Return the types of the columns as a list")

      // FromSequence must come first since it takes an object parameter
      // Otherwise, FromDict will never be called as object accepts anything
      .def("addRow", &addRowFromSequence, (arg("self"), arg("row_items_seq")),
           "Appends a row with the values from the given sequence. "
           "It it assumed that the items are in the correct order for the "
           "defined columns.")

      .def("addRow", &addRowFromDict, (arg("self"), arg("row_items_dict")),
           "Appends a row with the values from the dictionary.")

      .def("cell", &cell, (arg("self"), arg("value"), arg("row_or_column")),
           "Return the value in the given cell. If the value argument is a "
           "number then it is interpreted as a row otherwise it "
           "is interpreted as a column name.")

      .def("setCell", &setCell,
           (arg("self"), arg("row_or_column"), arg("column_or_row"), arg("value"), arg("notify_replace") = true),
           "Sets the value of a given cell. If the row_or_column argument is a "
           "number then it is interpreted as a row otherwise it "
           "is interpreted as a column name. If notify replace is false, then "
           "the replace workspace event is not triggered.")

      .def("toDict", &toDict, (arg("self")),
           "Gets the values of this workspace as a dictionary. The keys of the "
           "dictionary will be the names of the columns of the table. The "
           "values of the entries will be lists of values for each column.")

      .def("setColumnReadOnly", &setColumnReadOnly, (arg("self"), arg("column"), arg("read_only")),
           "Sets whether or not a given column of this workspace should be read-only. Columns can be "
           "selected by name or by index")

      .def("isColumnReadOnly", &isColumnReadOnly, (arg("self"), arg("column")),
           "Gets whether or not a given column of this workspace is be read-only. Columns can be "
           "selected by name or by index");

  //-------------------------------------------------------------------------------------------------

  RegisterWorkspacePtrToPython<ITableWorkspace>();
}
