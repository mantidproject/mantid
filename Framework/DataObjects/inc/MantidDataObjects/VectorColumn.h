// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_VECTORCOLUMN_H_
#define MANTID_DATAOBJECTS_VECTORCOLUMN_H_

#include "MantidAPI/Column.h"
#include "MantidKernel/StringTokenizer.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>

#include <cmath>
namespace Mantid {
namespace DataObjects {

/** VectorColumn : table column type capable of storing vectors of primitive
  types.

  Plese add more specializations to VectorColumn.cpp as you need them. I don't
  guarantee
  it will work correctly with complex or user types, but it might.
*/

template <class Type> class DLLExport VectorColumn : public API::Column {
public:
  VectorColumn() { m_type = typeName(); }

  /// Number of individual elements in the column
  size_t size() const override { return m_data.size(); }

  /// Returns typeid for the data in the column
  const std::type_info &get_type_info() const override {
    return typeid(std::vector<Type>);
  }

  /// Returns typeid for the pointer type to the data element in the column
  const std::type_info &get_pointer_type_info() const override {
    return typeid(std::vector<Type> *);
  }

  /// Print specified item to the stream
  void print(size_t index, std::ostream &s) const override {
    const std::vector<Type> &values = m_data.at(index);

    auto it = values.begin();

    if (it != values.end()) {
      s << *it;
      ++it;
    }

    for (; it != values.end(); ++it) {
      s << ',';
      s << *it;
    }
  }

  /// Set item from a string value
  void read(size_t index, const std::string &text) override {
    std::vector<Type> newValues;
    Mantid::Kernel::StringTokenizer elements(
        text, ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);

    for (const auto &element : elements) {
      try {
        newValues.push_back(boost::lexical_cast<Type>(element));
      } catch (boost::bad_lexical_cast &) {
        throw std::invalid_argument("Unable to convert one of the elements: " +
                                    element);
      }
    }

    m_data.at(index) = newValues;
  }

  /// Set item from a stream
  void read(const size_t index, std::istringstream &in) override {
    std::string s;
    in >> s;
    read(index, s);
  }

  /// Specialized type check
  bool isBool() const override { return false; }

  bool isNumber() const override { return false; }

  /// Overall memory size taken by the column (bytes)
  long int sizeOfData() const override {
    long int size(0);

    for (auto elemIt = m_data.begin(); elemIt != m_data.end(); ++elemIt) {
      size += static_cast<long int>(elemIt->size() * sizeof(Type));
    }

    return size;
  }

  /// Create another copy of the column
  VectorColumn *clone() const override {
    auto newColumn = new VectorColumn<Type>();
    newColumn->m_data = m_data;
    newColumn->setName(m_name);
    newColumn->setPlotType(m_plotType);
    return newColumn;
  }

  /// Cast to double
  double toDouble(size_t i) const override {
    UNUSED_ARG(i);
    throw std::runtime_error("VectorColumn is not convertible to double.");
  }

  /// Assign from double
  void fromDouble(size_t i, double value) override {
    UNUSED_ARG(i);
    UNUSED_ARG(value);
    throw std::runtime_error("VectorColumn is not assignable from double.");
  }

  /// Reference to the data.
  const std::vector<std::vector<Type>> &data() const { return m_data; }

  bool equals(const Column &otherColumn, double tolerance) const override {
    if (!possibleToCompare(otherColumn)) {
      return false;
    }
    const auto &otherColumnTyped =
        static_cast<const VectorColumn<Type> &>(otherColumn);
    const auto &otherData = otherColumnTyped.data();
    for (size_t i = 0; i < m_data.size(); i++) {
      if (m_data[i].size() != otherData[i].size()) {
        return false;
      }
      for (size_t j = 0; j < m_data[i].size(); j++) {
        if (fabs((double)m_data[i][j] - (double)otherData[i][j]) > tolerance) {
          return false;
        }
      }
    }
    return true;
  }

  bool equalsRelErr(const Column &otherColumn,
                    double tolerance) const override {
    if (!possibleToCompare(otherColumn)) {
      return false;
    }
    const auto &otherColumnTyped =
        static_cast<const VectorColumn<Type> &>(otherColumn);
    const auto &otherData = otherColumnTyped.data();
    for (size_t i = 0; i < m_data.size(); i++) {
      if (m_data[i].size() != otherData[i].size()) {
        return false;
      }
      for (size_t j = 0; j < m_data[i].size(); j++) {
        double num = fabs((double)m_data[i][j] - (double)otherData[i][j]);
        double den =
            (fabs((double)m_data[i][j]) + fabs((double)otherData[i][j])) / 2;
        if (den < tolerance && num > tolerance) {
          return false;
        } else if (num / den > tolerance) {
          return false;
        }
      }
    }
    return true;
  }

protected:
  /// Sets the new column size.
  void resize(size_t count) override { m_data.resize(count); }

  /// Inserts an item.
  void insert(size_t index) override {
    // Insert empty vector at the given position
    m_data.insert(m_data.begin() + index, std::vector<Type>());
  }

  /// Removes an item.
  void remove(size_t index) override { m_data.erase(m_data.begin() + index); }

  /// Pointer to a data element
  void *void_pointer(size_t index) override { return &m_data.at(index); }

  /// Pointer to a data element
  const void *void_pointer(size_t index) const override {
    return &m_data.at(index);
  }

private:
  /// All the vectors stored
  std::vector<std::vector<Type>> m_data;

  /// Returns the name of the column with the given type. Is specialized using
  /// DECLARE_VECTORCOLUMN
  std::string typeName();
};

} // namespace DataObjects
} // namespace Mantid

#define DECLARE_VECTORCOLUMN(Type, TypeName)                                   \
  template <> std::string VectorColumn<Type>::typeName() { return #TypeName; } \
  Kernel::RegistrationHelper register_column_##TypeName((                      \
      API::ColumnFactory::Instance().subscribe<VectorColumn<Type>>(#TypeName), \
      0));

#endif /* MANTID_DATAOBJECTS_VECTORCOLUMN_H_ */
