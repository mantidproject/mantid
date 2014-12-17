#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid {
namespace API {

/**
 */
const std::string ITableWorkspace::toString() const {
  std::ostringstream os;
  os << id() << "\n";
  os << "Columns: " << boost::lexical_cast<std::string>(columnCount()) << "\n";
  os << "Rows: " << boost::lexical_cast<std::string>(rowCount()) << "\n";
  os << getMemorySizeAsStr();
  return os.str();
}

/** Creates n new columns of the same type
 * @param type :: The datatype of the column
 * @param name :: The name to assign to the column
 * @param n :: The number of columns to create
 * @return True if the column was successfully added
 */
bool ITableWorkspace::addColumns(const std::string &type,
                                 const std::string &name, size_t n) {
  bool ok = true;
  for (size_t i = 0; i < n; i++) {
    std::ostringstream ostr;
    ostr << name << '_' << i;
    ok = ok && addColumn(type, ostr.str());
  }
  return ok;
}

/// Appends a row.
TableRowHelper ITableWorkspace::appendRow() {
  insertRow(rowCount());
  return getRow(rowCount() - 1);
}

/**
 * Access the column with name \c name trough a ColumnVector object
 * @param name :: The name of the column
 * @returns The named column
 */
TableColumnHelper ITableWorkspace::getVector(const std::string &name) {
  return TableColumnHelper(this, name);
}

/**
 * Access the column with name \c name trough a ColumnVector object (const)
 * @param name :: The name of the column
 * @returns The named column
 */
TableConstColumnHelper
ITableWorkspace::getVector(const std::string &name) const {
  return TableConstColumnHelper(this, name);
}

/**
 * If the workspace is the AnalysisDataService sends AfterReplaceNotification.
 */
void ITableWorkspace::modified() {
  if (!AnalysisDataService::Instance().doesExist(this->getName()))
    return;
  Workspace_sptr ws = AnalysisDataService::Instance().retrieve(this->getName());
  if (!ws)
    return;
  ITableWorkspace_sptr tws = boost::dynamic_pointer_cast<ITableWorkspace>(ws);
  if (!tws)
    return;
  AnalysisDataService::Instance().notificationCenter.postNotification(
      new Kernel::DataService<API::Workspace>::AfterReplaceNotification(
          this->getName(), tws));
}

/** Overridable method to custom-sort the workspace
 *
 * @param criteria : a vector with a list of pairs: column name, bool;
 *        where bool = true for ascending, false for descending sort.
 *        The peaks are sorted by the first criterion first, then the 2nd if
 *equal, etc.
 * @throw std::runtime_error unless overridden
 */
void
ITableWorkspace::sort(std::vector<std::pair<std::string, bool>> &criteria) {
  UNUSED_ARG(criteria);
  throw std::runtime_error("This type of ITableWorkspace (" + this->id() +
                           ") has not implemented sort() yet customSort() "
                           "returns true. Please contact the developers.");
}

} // namespace API
} // Namespace Mantid

///\cond TEMPLATE
namespace Mantid {
namespace Kernel {
template <>
MANTID_API_DLL API::ITableWorkspace_sptr
IPropertyManager::getValue<API::ITableWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<API::ITableWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<API::ITableWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected ITableWorkspace";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL API::ITableWorkspace_const_sptr
IPropertyManager::getValue<API::ITableWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<API::ITableWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<API::ITableWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected const ITableWorkspace";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
