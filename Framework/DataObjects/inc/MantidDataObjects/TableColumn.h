// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>

#include "MantidAPI/Column.h"
#include "MantidKernel/FloatingPointComparison.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace DataObjects {

/** \class TableColumn

    Class TableColumn implements abstract class Column for any copyable data
   type.
    A TableColumn is created using TableWorkspace::addColumn(type,name).
    type is the simbolic name of the data type which must be first declared with
    DECLARE_TABLECOLUMN macro. Predeclared types are:

    "int"    for int
    "int32_t" for int32_t
    "size_t" for size_t
    "float"  for float
    "double" for double
    "bool"   for Boolean
    "str"    for std::string
    "V3D"    for Mantid::Kernel::V3D

    Boolean is used instead of bool because of bool's non-standard treatmemt in
   std::vector.

    \author Roman Tolchenov
    \date 31/10/2008
*/
template <class Type> class TableColumn : public API::Column {
  /// Helper struct helping to write a generic casting to double
  struct InconvertibleToDoubleType {
    /// Constructor
    InconvertibleToDoubleType(const Type &) {}
    /// Constructor
    InconvertibleToDoubleType(const double &) {}
    /// Convertion to double throws a runtime_error.
    operator double() const {
      throw std::runtime_error(std::string("Cannot convert ") + typeid(Type).name() + " to double.");
    }
    operator Type() const {
      throw std::runtime_error(std::string("Cannot convert double to ") + typeid(Type).name() + ".");
    }
  };

public:
  TableColumn() {
    int length = sizeof(Type);
    std::string name = std::string(typeid(Type).name());
    if ((name.find('i') != std::string::npos) || (name.find('l') != std::string::npos) ||
        (name.find('x') != std::string::npos)) {
      if (length == 4) { // cppcheck-suppress knownConditionTrueFalse
        this->m_type = "int";
      }
      if (length == 8) { // cppcheck-suppress knownConditionTrueFalse
        this->m_type = "int64";
      }
    }
    if (name.find('f') != std::string::npos) {
      this->m_type = "float";
    }
    if (name.find('d') != std::string::npos) {
      this->m_type = "double";
    }
    if (name.find('u') != std::string::npos) {
      if (length == 4) { // cppcheck-suppress knownConditionTrueFalse
        this->m_type = "uint32_t";
      }
      if (length == 8) { // cppcheck-suppress knownConditionTrueFalse
        this->m_type = "uint64_t";
      }
    }
    if (this->m_type.empty()) {
      this->m_type = name;
    }
  }

  // TableColumn();
  /// Number of individual elements in the column.
  size_t size() const override { return m_data.size(); }
  /// Type id of the data in the column
  const std::type_info &get_type_info() const override { return typeid(Type); }
  /// Type id of the pointer to data in the column
  const std::type_info &get_pointer_type_info() const override { return typeid(Type *); }
  /// Output to an ostream.
  void print(size_t index, std::ostream &s) const override { s << m_data[index]; }
  /// Read in a string and set the value at the given index
  void read(size_t index, const std::string &text) override;
  /// Read in from stream and set the value at the given index
  void read(const size_t index, std::istringstream &in) override;
  /// Type check
  bool isBool() const override { return typeid(Type) == typeid(API::Boolean); }
  bool isNumber() const override { return std::is_convertible<Type, double>::value; }
  /// Memory used by the column
  long int sizeOfData() const override { return static_cast<long int>(m_data.size() * sizeof(Type)); }
  /// Clone
  TableColumn *clone() const override { return new TableColumn(*this); }

  /**
   * Cast an element to double if possible. If it's impossible
   * boost::numeric::bad_numeric_cast
   * is throw. In case of an overflow boost::numeric::positive_overflow or
   * boost::numeric::negative_overflow
   * is throw.
   * @param value :: The value of the element.
   */
  template <typename T> double convertToDouble(const T &value) const {
    using DoubleType =
        typename std::conditional<std::is_convertible<double, T>::value, T, InconvertibleToDoubleType>::type;
    return boost::numeric_cast<double, DoubleType>(value);
  }

  /**
   * Cast an string to double if possible. If it's impossible
   * std::invalid_argument
   * is throw. In case of an overflow boost::numeric::positive_overflow or
   * boost::numeric::negative_overflow
   * is throw.
   * @param value :: The value of the element.
   */

  double convertToDouble(const std::string &value) const { return std::stod(value); }

  double toDouble(size_t i) const override { return convertToDouble(m_data[i]); }

  /**
   * Cast an element to double if possible. If it's impossible
   * boost::numeric::bad_numeric_cast
   * is throw. In case of an overflow boost::numeric::positive_overflow or
   * boost::numeric::negative_overflow
   * is throw.
   * @param i :: The index to an element.
   * @param value: cast this value
   */
  void fromDouble(size_t i, double value) override {
    using DoubleType =
        typename std::conditional<std::is_convertible<double, Type>::value, Type, InconvertibleToDoubleType>::type;
    m_data[i] = static_cast<Type>(boost::numeric_cast<DoubleType, double>(value));
  }

  /// Reference to the data.
  std::vector<Type> &data() { return m_data; }
  /// Const reference to the data.
  const std::vector<Type> &data() const { return m_data; }
  /// Pointer to the data array
  Type *dataArray() { return &m_data[0]; }

  /// return a value casted to double; the users responsibility is to be sure,
  /// that the casting is possible
  double operator[](size_t i) const override {
    try {
      return convertToDouble(m_data[i]);
    } catch (...) {
      return std::numeric_limits<double>::quiet_NaN();
    }
  }

  /// Sort a vector of indices according to values in corresponding cells of
  /// this column.
  void sortIndex(bool ascending, size_t start, size_t end, std::vector<size_t> &indexVec,
                 std::vector<std::pair<size_t, size_t>> &equalRanges) const override;

  /// Re-arrange values in this column according to indices in indexVec
  void sortValues(const std::vector<size_t> &indexVec) override;

  bool equals(const Column &otherColumn, double tolerance, bool const nanEqual = false) const override {
    if (!possibleToCompare(otherColumn)) {
      return false;
    }
    const auto &otherColumnTyped = static_cast<const TableColumn<Type> &>(otherColumn);
    const auto &otherData = otherColumnTyped.data();
    return compareVectors(otherData, tolerance, nanEqual);
  }

  bool equalsRelErr(const Column &otherColumn, double tolerance, bool const nanEqual = false) const override {
    if (!possibleToCompare(otherColumn)) {
      return false;
    }
    const auto &otherColumnTyped = static_cast<const TableColumn<Type> &>(otherColumn);
    const auto &otherData = otherColumnTyped.data();
    return compareVectorsRelError(otherData, tolerance, nanEqual);
  }

protected:
  /// Resize.
  void resize(size_t count) override { m_data.resize(count); }
  /// Inserts default value at position index.
  void insert(size_t index) override {
    if (index < m_data.size())
      m_data.insert(m_data.begin() + index, Type());
    else
      m_data.emplace_back();
  }
  /// Removes an item at index.
  void remove(size_t index) override { m_data.erase(m_data.begin() + index); }
  /// Returns a pointer to the data element.
  void *void_pointer(size_t index) override { return &m_data.at(index); }
  /// Returns a pointer to the data element.
  const void *void_pointer(size_t index) const override { return &m_data.at(index); }

private:
  /// Column data
  std::vector<Type> m_data;
  friend class TableWorkspace;

  // helper function template for equality
  bool compareVectors(const std::vector<Type> &newVector, double tolerance, bool const nanEqual = false) const {
    return compareVectors(newVector, tolerance, nanEqual, std::is_integral<Type>());
  }

  bool compareVectors(const std::vector<Type> &newVector, double tolerance, bool const, std::true_type) const {
    for (size_t i = 0; i < m_data.size(); i++) {
      if (!Kernel::withinAbsoluteDifference<Type, double>(m_data[i], newVector[i], tolerance)) {
        return false;
      }
    }
    return true;
  }

  bool compareVectors(const std::vector<Type> &newVector, double tolerance, bool const nanEqual,
                      std::false_type) const {
    for (size_t i = 0; i < m_data.size(); i++) {
      if (nanEqual && std::isnan(m_data[i]) && std::isnan(newVector[i])) {
        continue;
      } else if (!Kernel::withinAbsoluteDifference<Type, double>(m_data[i], newVector[i], tolerance)) {
        return false;
      }
    }
    return true;
  }

  // helper function template for equality with relative error
  bool compareVectorsRelError(const std::vector<Type> &newVector, double tolerance, bool const nanEqual = false) const {
    return compareVectorsRelError(newVector, tolerance, nanEqual, std::is_integral<Type>());
  }

  bool compareVectorsRelError(const std::vector<Type> &newVector, double tolerance, bool const, std::true_type) const {
    for (size_t i = 0; i < m_data.size(); i++) {
      if (!Kernel::withinRelativeDifference<Type, double>(m_data[i], newVector[i], tolerance)) {
        return false;
      }
    }
    return true;
  }

  bool compareVectorsRelError(const std::vector<Type> &newVector, double tolerance, bool const nanEqual,
                              std::false_type) const {
    for (size_t i = 0; i < m_data.size(); i++) {
      if (nanEqual && std::isnan(m_data[i]) && std::isnan(newVector[i])) {
        continue;
      } else if (!Kernel::withinRelativeDifference<Type, double>(m_data[i], newVector[i], tolerance)) {
        return false;
      }
    }
    return true;
  }
};

/// Template specialisation for strings for comparison
template <>
inline bool TableColumn<std::string>::compareVectors(const std::vector<std::string> &newVector, double,
                                                     bool const) const {
  for (size_t i = 0; i < m_data.size(); i++) {
    if (m_data[i] != newVector[i]) {
      return false;
    }
  }
  return true;
}

/// Template specialisation for strings for comparison
template <>
inline bool TableColumn<API::Boolean>::compareVectors(const std::vector<API::Boolean> &newVector, double,
                                                      bool const) const {
  for (size_t i = 0; i < m_data.size(); i++) {
    if (!(m_data[i] == newVector[i])) {
      return false;
    }
  }
  return true;
}

/// Template specialisation for V3D for comparison
template <>
inline bool TableColumn<Kernel::V3D>::compareVectors(const std::vector<Kernel::V3D> &newVector, double tolerance,
                                                     bool const) const {
  for (size_t i = 0; i < m_data.size(); i++) {
    double dif_x = fabs(m_data[i].X() - newVector[i].X());
    double dif_y = fabs(m_data[i].Y() - newVector[i].Y());
    double dif_z = fabs(m_data[i].Z() - newVector[i].Z());
    if (dif_x > tolerance || dif_y > tolerance || dif_z > tolerance) {
      return false;
    }
  }
  return true;
}

/// Template specialisation for strings for comparison
template <>
inline bool TableColumn<std::string>::compareVectorsRelError(const std::vector<std::string> &newVector,
                                                             double tolerance, bool const) const {
  return compareVectors(newVector, tolerance);
}

/// Template specialisation for bools for comparison
template <>
inline bool TableColumn<API::Boolean>::compareVectorsRelError(const std::vector<API::Boolean> &newVector,
                                                              double tolerance, bool const) const {
  return compareVectors(newVector, tolerance);
}

/// Template specialisation for V3D for comparison
template <>
inline bool TableColumn<Kernel::V3D>::compareVectorsRelError(const std::vector<Kernel::V3D> &newVector,
                                                             double tolerance, bool const) const {
  for (size_t i = 0; i < m_data.size(); i++) {
    double dif_x = fabs(m_data[i].X() - newVector[i].X());
    double dif_y = fabs(m_data[i].Y() - newVector[i].Y());
    double dif_z = fabs(m_data[i].Z() - newVector[i].Z());
    double den_x = 0.5 * (fabs(m_data[i].X()) + fabs(newVector[i].X()));
    double den_y = 0.5 * (fabs(m_data[i].X()) + fabs(newVector[i].X()));
    double den_z = 0.5 * (fabs(m_data[i].X()) + fabs(newVector[i].X()));
    if (den_x > tolerance || den_y > tolerance || den_z > tolerance) {
      if (dif_x / den_x > tolerance || dif_y / den_y > tolerance || dif_z / den_z > tolerance) {
        return false;
      }
    } else {
      if (dif_x > tolerance || dif_y > tolerance || dif_z > tolerance) {
        return false;
      }
    }
  }
  return true;
}

/// Template specialization for strings so they can contain spaces
template <> inline void TableColumn<std::string>::read(size_t index, const std::string &text) {
  /* As opposed to other types, assigning strings via a stream does not work if
   * it contains a whitespace character, so instead the assignment operator is
   * used.
   */
  m_data[index] = text;
}

/// Template specialization for strings so they can contain spaces
template <> inline void TableColumn<std::string>::read(size_t index, std::istringstream &text) {
  /* As opposed to other types, assigning strings via a stream does not work if
   * it contains a whitespace character, so instead the assignment operator is
   * used.
   */
  m_data[index] = text.str();
}

/// Read in a string and set the value at the given index
template <typename Type> void TableColumn<Type>::read(size_t index, const std::string &text) {
  std::istringstream istr(text);
  istr >> m_data[index];
}

/// Read in from stream and set the value at the given index
template <typename Type> void TableColumn<Type>::read(size_t index, std::istringstream &in) {
  Type t;
  in >> t;
  m_data[index] = t;
}

namespace {
/// Comparison object to compare column values given their indices.
template <typename Type> class CompareValues {
  const std::vector<Type> &m_data;
  const bool m_ascending;

public:
  CompareValues(const TableColumn<Type> &column, bool ascending) : m_data(column.data()), m_ascending(ascending) {}
  bool operator()(size_t i, size_t j) {
    return m_ascending ? m_data[i] < m_data[j] : !(m_data[i] < m_data[j] || m_data[i] == m_data[j]);
  }
};
} // namespace

/// Sort a vector of indices according to values in corresponding cells of this
/// column. @see Column::sortIndex
template <typename Type>
void TableColumn<Type>::sortIndex(bool ascending, size_t start, size_t end, std::vector<size_t> &indexVec,
                                  std::vector<std::pair<size_t, size_t>> &equalRanges) const {
  equalRanges.clear();

  const size_t n = m_data.size();
  if (n == 0) {
    return;
  }

  auto iBegin = indexVec.begin() + start;
  auto iEnd = indexVec.begin() + end;

  std::stable_sort(iBegin, iEnd, CompareValues<Type>(*this, ascending));

  bool same = false;
  size_t eqStart = 0;
  for (auto i = iBegin + 1; i != iEnd; ++i) {
    if (!same) {
      if (m_data[*i] == m_data[*(i - 1)]) {
        eqStart = static_cast<size_t>(std::distance(indexVec.begin(), i - 1));
        same = true;
      }
    } else {
      if (m_data[*i] != m_data[*(i - 1)]) {
        auto p = std::make_pair(eqStart, static_cast<size_t>(std::distance(indexVec.begin(), i)));
        equalRanges.emplace_back(p);
        same = false;
      }
    }
  }

  // last elements are equal
  if (same) {
    auto p = std::make_pair(eqStart, static_cast<size_t>(std::distance(indexVec.begin(), iEnd)));
    equalRanges.emplace_back(p);
  }
}

/// Re-arrange values in this column in order of indices in indexVec
template <typename Type> void TableColumn<Type>::sortValues(const std::vector<size_t> &indexVec) {
  assert(m_data.size() == indexVec.size());
  std::vector<Type> sortedData(m_data.size());

  auto sortedIt = sortedData.begin();
  for (auto idx = indexVec.begin(); idx != indexVec.end(); ++idx, ++sortedIt) {
    *sortedIt = m_data[*idx];
  }

  std::swap(m_data, sortedData);
}

template <> inline double TableColumn<API::Boolean>::toDouble(size_t i) const { return m_data[i] ? 1.0 : 0.0; }

template <> inline void TableColumn<API::Boolean>::fromDouble(size_t i, double value) { m_data[i] = value != 0.0; }

/// Shared pointer to a column with automatic type cast and data type check.
/// Can be created with TableWorkspace::getColumn(...)
template <class T> class TableColumn_ptr : public std::shared_ptr<TableColumn<T>> {
public:
  /** Constructor
      @param c :: Shared pointer to a column
    */
  TableColumn_ptr(std::shared_ptr<API::Column> c)
      : std::shared_ptr<TableColumn<T>>(std::dynamic_pointer_cast<TableColumn<T>>(c)) {
    if (!this->get()) {
      std::string str = "Data type of column " + c->name() + " does not match " + typeid(T).name();
      throw std::runtime_error(str);
    }
  }
};

/// Special case of bool
template <> class TableColumn_ptr<bool> : public TableColumn_ptr<API::Boolean> {
public:
  /** Constructor
      @param c :: Shared pointer to a column
    */
  TableColumn_ptr(const std::shared_ptr<API::Column> &c) : TableColumn_ptr<API::Boolean>(c) {
    if (!this->get()) {
      std::string str = "Data type of column " + c->name() + " does not match " + typeid(API::Boolean).name();
      throw std::runtime_error(str);
    }
  }
};

} // namespace DataObjects
} // Namespace Mantid

/*
    Macro to declare a type to be used with TableColumn.
    DataType is the actual C++ type. TypeName is a symbolic name, used in
   TableWorkspace::createColumn(...)
    TypeName can contain only letters, numbers and _s.
*/
#define DECLARE_TABLECOLUMN(DataType, TypeName)                                                                        \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper register_column_##TypeName(                                                       \
      (Mantid::API::ColumnFactory::Instance().subscribe<Mantid::DataObjects::TableColumn<DataType>>(#TypeName), 0));   \
  }
