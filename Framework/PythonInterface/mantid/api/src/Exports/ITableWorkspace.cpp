#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/kernel/Converters/CloneToNumpy.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/NdArray.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include "MantidPythonInterface/kernel/Registry/RegisterWorkspacePtrToPython.h"

#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/python/class.hpp>
#include <boost/python/converter/builtin_converters.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <cstring>
#include <vector>

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using namespace Mantid::API;
using namespace Mantid::PythonInterface::Policies;
using Mantid::PythonInterface::Registry::RegisterWorkspacePtrToPython;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ITableWorkspace)

namespace {
namespace bpl = boost::python;
namespace Converters = Mantid::PythonInterface::Converters;
namespace NumPy = Mantid::PythonInterface::NumPy;

// Numpy PyArray_IsIntegerScalar is broken for Python 3 for numpy < 1.11
#if PY_MAJOR_VERSION >= 3
#define TO_LONG PyLong_AsLong
#define STR_CHECK PyUnicode_Check
#if NPY_API_VERSION < 0x0000000a //(1.11)
#define IS_ARRAY_INTEGER(obj)                                                  \
  (PyLong_Check(obj) || PyArray_IsScalar((obj), Integer))
#else
#define IS_ARRAY_INTEGER PyArray_IsIntegerScalar
#endif
#else // Python 2
#define IS_ARRAY_INTEGER PyArray_IsIntegerScalar
#define TO_LONG PyInt_AsLong
#define STR_CHECK PyString_Check
#endif

/// Boost macro for "looping" over builtin types
#define BUILTIN_TYPES                                                          \
  BOOST_PP_TUPLE_TO_LIST(                                                      \
      7, (double, std::string, int, uint32_t, int64_t, float, uint64_t))
#define USER_TYPES BOOST_PP_TUPLE_TO_LIST(1, (Mantid::Kernel::V3D))
#define ARRAY_TYPES                                                            \
  BOOST_PP_TUPLE_TO_LIST(2, (std::vector<int>, std::vector<double>))

/**
 * Get out the Python value from a specific cell of the supplied column. This is
 * meant to
 * reduce the amount of copy and pasted code in this file.
 * @param column The column to grab the value from.
 * @param typeID The python identifier of the column type.
 * @param row The row to get the value from.
 */
PyObject *getValue(Mantid::API::Column_const_sptr column,
                   const std::type_info &typeID, const int row) {
  if (typeID.hash_code() == typeid(Mantid::API::Boolean).hash_code()) {
    bool res = column->cell<Mantid::API::Boolean>(row);
    return to_python_value<const bool &>()(res);
  }

#define GET_BUILTIN(R, _, T)                                                   \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                      \
    result = to_python_value<const T &>()(column->cell<T>(row));               \
  }
#define GET_USER(R, _, T)                                                      \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                      \
    const converter::registration *entry =                                     \
        converter::registry::query(typeid(T));                                 \
    if (!entry)                                                                \
      throw std::invalid_argument("Cannot find converter from C++ type.");     \
    result = entry->to_python((const void *)&column->cell<T>(row));            \
  }
#define GET_ARRAY(R, _, T)                                                     \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                      \
    result = Converters::Clone::apply<T::value_type>::create1D(                \
        column->cell<T>(row));                                                 \
  }

  // -- Use the boost preprocessor to generate a list of else if clause to cut
  // out copy
  // and pasted code.
  // cppcheck-suppress unreadVariable
  PyObject *result(nullptr);
  if (false) {
  } // So that it always falls through to the list checking
  BOOST_PP_LIST_FOR_EACH(GET_BUILTIN, _, BUILTIN_TYPES)
  BOOST_PP_LIST_FOR_EACH(GET_ARRAY, _, ARRAY_TYPES)
  BOOST_PP_LIST_FOR_EACH(GET_USER, _, USER_TYPES)
  else {
    throw std::invalid_argument("Cannot convert C++ type to Python: " +
                                column->type());
  }
  return result;
}

/**
 * Sets a value in a particular column and row from a python object
 * @param column :: A pointer to the column object
 * @param row :: The index of the row
 * @param value :: The value to set
 */
void setValue(const Column_sptr column, const int row,
              const bpl::object &value) {
  const auto &typeID = column->get_type_info();

  // Special case: Treat Mantid Boolean as normal bool
  if (typeID.hash_code() == typeid(Mantid::API::Boolean).hash_code()) {
    column->cell<Mantid::API::Boolean>(row) = bpl::extract<bool>(value)();
    return;
  }

  // Special case: Boost has issues with NumPy ints, so use Python API instead
  // to check this first
  if (typeID.hash_code() == typeid(int).hash_code() &&
      IS_ARRAY_INTEGER(value.ptr())) {
    column->cell<int>(row) = static_cast<int>(TO_LONG(value.ptr()));
    return;
  }

// Macros for all other types
#define SET_CELL(R, _, T)                                                      \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                      \
    column->cell<T>(row) = bpl::extract<T>(value)();                           \
  }
#define SET_VECTOR_CELL(R, _, T)                                               \
  else if (typeID.hash_code() == typeid(T).hash_code()) {                      \
    if (!NumPy::NdArray::check(value)) {                                       \
      column->cell<T>(row) =                                                   \
          Converters::PySequenceToVector<T::value_type>(value)();              \
    } else {                                                                   \
      column->cell<T>(row) =                                                   \
          Converters::NDArrayToVector<T::value_type>(value)();                 \
    }                                                                          \
  }

  // -- Use the boost preprocessor to generate a list of else if clause to cut
  // out copy and pasted code.
  if (false) {
  } // So that it always falls through to the list checking
  BOOST_PP_LIST_FOR_EACH(SET_CELL, _, BUILTIN_TYPES)
  BOOST_PP_LIST_FOR_EACH(SET_CELL, _, USER_TYPES)
  BOOST_PP_LIST_FOR_EACH(SET_VECTOR_CELL, _, ARRAY_TYPES)
  else {
    throw std::invalid_argument("Cannot convert Python type to C++: " +
                                column->type());
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
bool addColumnPlotType(ITableWorkspace &self, const std::string &type,
                       const std::string &name, int plottype) {
  auto column = self.addColumn(type, name);

  if (column)
    column->setPlotType(plottype);

  return column != nullptr;
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
bool addColumnSimple(ITableWorkspace &self, const std::string &type,
                     const std::string &name) {
  return self.addColumn(type, name) != nullptr;
}

/**
 * Get the plot type of a column given by name or index
 * @param self Reference to TableWorkspace this is called on
 * @param column Name or index of column
 * @return PlotType: 0=None, 1=X, 2=Y, 3=Z, 4=xErr, 5=yErr, 6=Label
 */
int getPlotType(ITableWorkspace &self, const bpl::object &column) {
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
 */
void setPlotType(ITableWorkspace &self, const bpl::object &column, int ptype) {
  // Find the column
  Mantid::API::Column_sptr colptr;
  if (STR_CHECK(column.ptr())) {
    colptr = self.getColumn(extract<std::string>(column)());
  } else {
    colptr = self.getColumn(extract<int>(column)());
  }

  colptr->setPlotType(ptype);
}

/**
 * Access a cell and return a corresponding Python type
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @param value A python object containing a column name or index
 */
PyObject *column(const ITableWorkspace &self, const bpl::object &value) {
  // Find the column and row
  Mantid::API::Column_const_sptr column;
  if (STR_CHECK(value.ptr())) {
    column = self.getColumn(extract<std::string>(value)());
  } else {
    column = self.getColumn(extract<int>(value)());
  }
  const std::type_info &typeID = column->get_type_info();
  const int numRows = static_cast<int>(column->size());

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
    throw std::invalid_argument(
        "Cannot specify row larger than number of rows");

  int numCols = static_cast<int>(self.columnCount());

  PyObject *result = PyDict_New();

  for (int col = 0; col < numCols; col++) {
    Mantid::API::Column_const_sptr column = self.getColumn(col);
    const std::type_info &typeID = column->get_type_info();

    if (PyDict_SetItemString(result, column->name().c_str(),
                             getValue(column, typeID, row)))
      throw std::runtime_error("Error while building dict");
  }

  return result;
}

/**
 * Adds a new row in the table, where the items are given in a dictionary
 * object mapping {column name:value}. It must contain a key-value entry for
 * every column in the row, otherwise the insert will fail.
 *
 * @param self :: A reference to the ITableWorkspace object
 * @param rowItems :: A dictionary defining the items in the row
 */
void addRowFromDict(ITableWorkspace &self, const bpl::dict &rowItems) {
  // rowItems must contain an entry for every column
  bpl::ssize_t nitems = boost::python::len(rowItems);
  if (nitems != static_cast<bpl::ssize_t>(self.columnCount())) {
    throw std::invalid_argument(
        "Number of values given does not match the number of columns. "
        "Expected: " +
        std::to_string(self.columnCount()));
  }

  // Add a new row to populate with values
  const int rowIndex = static_cast<int>(self.rowCount());
  self.appendRow();

  // Declared in this scope so we can access them in catch block
  Column_sptr column; // Column in table
  bpl::object value;  // Value from dictionary

  try {
    // Retrieve and set the value for each column
    auto columns = self.getColumnNames();
    for (auto &iter : columns) {
      column = self.getColumn(iter);
      value = rowItems[iter];
      setValue(column, rowIndex, value);
    }
  } catch (bpl::error_already_set &) {
    // One of the columns wasn't found in the dictionary
    if (PyErr_ExceptionMatches(PyExc_KeyError)) {
      std::ostringstream msg;
      msg << "Missing key-value entry for column ";
      msg << "<" << column->name() << ">";
      PyErr_SetString(PyExc_KeyError, msg.str().c_str());
    }

    // Wrong type of data for one of the columns
    if (PyErr_ExceptionMatches(PyExc_TypeError)) {
      std::ostringstream msg;
      msg << "Wrong datatype <";
      msg << std::string(
          bpl::extract<std::string>(value.attr("__class__").attr("__name__")));
      msg << "> for column <" << column->name() << "> ";
      msg << "(expected <" << column->type() << ">)";
      PyErr_SetString(PyExc_TypeError, msg.str().c_str());
    }

    // Remove the new row since populating it has failed
    self.removeRow(rowIndex);
    throw;
  }
}

/**
 * Adds a new row in the table, where the items are given in an ordered python
 * sequence type (list, tuple, numpy array, etc). It must be the same length as
 * the number of columns or the insert will fail.
 *
 * @param self :: A reference to the ITableWorkspace object
 * @param rowItems :: A sequence containing the column values in the row
 */
void addRowFromSequence(ITableWorkspace &self, const bpl::object &rowItems) {
  // rowItems must contain an entry for every column
  bpl::ssize_t nitems = boost::python::len(rowItems);
  if (nitems != static_cast<bpl::ssize_t>(self.columnCount())) {
    throw std::invalid_argument(
        "Number of values given does not match the number of columns. "
        "Expected: " +
        std::to_string(self.columnCount()));
  }

  // Add a new row to populate with values
  const int rowIndex = static_cast<int>(self.rowCount());
  self.appendRow();

  // Loop over sequence and set each column value in same order
  for (bpl::ssize_t i = 0; i < nitems; ++i) {
    auto column = self.getColumn(i);
    auto value = rowItems[i];

    try {
      setValue(column, rowIndex, value);
    } catch (bpl::error_already_set &) {
      // Wrong type of data for one of the columns
      if (PyErr_ExceptionMatches(PyExc_TypeError)) {
        std::ostringstream msg;
        msg << "Wrong datatype <";
        msg << std::string(bpl::extract<std::string>(
            value.attr("__class__").attr("__name__")));
        msg << "> for column <" << column->name() << "> ";
        msg << "(expected <" << column->type() << ">)";
        PyErr_SetString(PyExc_TypeError, msg.str().c_str());
      }

      // Remove the new row since populating it has failed
      self.removeRow(rowIndex);
      throw;
    }
  }
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
void getCellLoc(ITableWorkspace &self, const bpl::object &col_or_row,
                const int row_or_col, Column_sptr &column, int &rowIndex) {
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
PyObject *cell(ITableWorkspace &self, const bpl::object &value,
               int row_or_col) {
  // Find the column and row
  Mantid::API::Column_sptr column;
  int row(-1);
  getCellLoc(self, value, row_or_col, column, row);
  const std::type_info &typeID = column->get_type_info();
  return getValue(column, typeID, row);
}

/**
 * Sets the value of the given cell
 * @param self A reference to the TableWorkspace python object that we were
 * called on
 * @param value A python object containing either a row index or a column name
 * @param row_or_col An integer giving the row if value is a string or the
 * column if value is an index
 */
void setCell(ITableWorkspace &self, const bpl::object &col_or_row,
             const int row_or_col, const bpl::object &value) {
  Mantid::API::Column_sptr column;
  int row(-1);
  getCellLoc(self, col_or_row, row_or_col, column, row);
  setValue(column, row, value);
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
bpl::dict toDict(const ITableWorkspace &self) {
  bpl::dict result;

  for (const auto &name : self.getColumnNames()) {
    bpl::handle<> handle(column(self, bpl::object(name)));
    bpl::object values(handle);
    result[name] = values;
  }

  return result;
}

/** Constructor function for ITableWorkspaces */
ITableWorkspace_sptr makeTableWorkspace() {
  const auto ws = WorkspaceFactory::Instance().createTable();
  Mantid::API::AnalysisDataService::Instance().add(ws->getName(), ws);
  return ws;
}

class ITableWorkspacePickleSuite : public boost::python::pickle_suite {
public:
  static dict getstate(const ITableWorkspace &ws) {
    dict data;
    data["data"] = toDict(ws);
    data["meta_data"] = writeMetaData(ws);
    return data;
  }

  static void setstate(ITableWorkspace &ws, dict state) {
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

    const bpl::ssize_t numColumns = len(columnNames);
    for (bpl::ssize_t colIndex = 0; colIndex < numColumns; ++colIndex) {
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

    bpl::ssize_t numRows = len(data[names[0]]);
    for (int rowIndex = 0; rowIndex < numRows; ++rowIndex) {
      ws.appendRow();
      for (const auto &name : names) {
        setValue(ws.getColumn(name), static_cast<int>(rowIndex),
                 data[name][rowIndex]);
      }
    }
  }
};

void export_ITableWorkspace() {
  using Mantid::PythonInterface::Policies::VectorToNumpy;

  std::string iTableWorkspace_docstring =
      "Most of the information from a table workspace is returned ";
  iTableWorkspace_docstring +=
      "as native copies. All of the column accessors return lists while the ";
  iTableWorkspace_docstring +=
      "rows return dicts. This object does support the idom 'for row in ";
  iTableWorkspace_docstring += "ITableWorkspace'.";

  class_<ITableWorkspace, bases<Workspace>, boost::noncopyable>(
      "ITableWorkspace", iTableWorkspace_docstring.c_str(), no_init)
      .def_pickle(ITableWorkspacePickleSuite())
      .def("__init__", make_constructor(&makeTableWorkspace))
      .def("addColumn", &addColumnSimple,
           (arg("self"), arg("type"), arg("name")),
           "Add a named column with the given type. Recognized types are: "
           "int,float,double,bool,str,V3D,long64")

      .def("addColumn", &addColumnPlotType,
           (arg("self"), arg("type"), arg("name"), arg("plottype")),
           "Add a named column with the given datatype "
           "(int,float,double,bool,str,V3D,long64) "
           "\nand plottype "
           "(0 = None, 1 = X, 2 = Y, 3 = Z, 4 = xErr, 5 = yErr, 6 = Label).")

      .def("getPlotType", &getPlotType, (arg("self"), arg("column")),
           "Get the plot type of given column as an integer. "
           "Accepts column name or index. \nPossible return values: "
           "(0 = None, 1 = X, 2 = Y, 3 = Z, 4 = xErr, 5 = yErr, 6 = Label).")

      .def("setPlotType", &setPlotType,
           (arg("self"), arg("column"), arg("ptype")),
           "Set the plot type of given column. "
           "Accepts column name or index. \nPossible type values: "
           "(0 = None, 1 = X, 2 = Y, 3 = Z, 4 = xErr, 5 = yErr, 6 = Label).")

      .def("removeColumn", &ITableWorkspace::removeColumn,
           (arg("self"), arg("name")), "Remove the named column.")

      .def("columnCount", &ITableWorkspace::columnCount, arg("self"),
           "Returns the number of columns in the workspace.")

      .def("rowCount", &ITableWorkspace::rowCount, arg("self"),
           "Returns the number of rows within the workspace.")

      .def("setRowCount", &ITableWorkspace::setRowCount,
           (arg("self"), arg("count")),
           "Resize the table to contain count rows.")

      .def("__len__", &ITableWorkspace::rowCount, arg("self"),
           "Returns the number of rows within the workspace.")

      .def("getColumnNames", &ITableWorkspace::getColumnNames, arg("self"),
           boost::python::return_value_policy<VectorToNumpy>(),
           "Return a list of the column names.")

      .def("keys", &ITableWorkspace::getColumnNames, arg("self"),
           boost::python::return_value_policy<VectorToNumpy>(),
           "Return a list of the column names.")

      .def("column", &column, (arg("self"), arg("column")),
           "Return all values of a specific column as a list.")

      .def("row", &row, (arg("self"), arg("row")),
           "Return all values of a specific row as a dict.")

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
           (arg("self"), arg("row_or_column"), arg("column_or_row"),
            arg("value")),
           "Sets the value of a given cell. If the row_or_column argument is a "
           "number then it is interpreted as a row otherwise it "
           "is interpreted as a column name.")

      .def("toDict", &toDict, (arg("self")),
           "Gets the values of this workspace as a dictionary. The keys of the "
           "dictionary will be the names of the columns of the table. The "
           "values of the entries will be lists of values for each column.");

  //-------------------------------------------------------------------------------------------------

  RegisterWorkspacePtrToPython<ITableWorkspace>();
}
