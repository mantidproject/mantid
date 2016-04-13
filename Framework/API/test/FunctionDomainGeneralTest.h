#ifndef FUNCTIONDOMAINGENERALTEST_H_
#define FUNCTIONDOMAINGENERALTEST_H_

#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidAPI/Column.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

namespace {

template <typename T> class TestColumn : public Column {
public:
  /// Constructor
  TestColumn(size_t n) : m_data(n) {}
  /// Number of individual elements in the column.
  virtual size_t size() const { return m_data.size(); }
  /// Returns typeid for the data in the column
  virtual const std::type_info &get_type_info() const { return typeid(T); }
  /// Returns typeid for the pointer type to the data element in the column
  virtual const std::type_info &get_pointer_type_info() const {
    return typeid(T *);
  }
  /// Prints out the value to a stream
  virtual void print(size_t, std::ostream &) const {
    throw std::logic_error("Not implemented");
  }
  /// Specialized type check
  virtual bool isBool() const { return false; }
  /// Must return overall memory size taken by the column.
  virtual long int sizeOfData() const {
    throw std::logic_error("Not implemented");
  }
  /// Virtual constructor. Fully clone any column.
  virtual Column *clone() const { throw std::logic_error("Not implemented"); }
  /// Cast an element to double if possible
  virtual double toDouble(size_t) const {
    throw std::logic_error("Not implemented");
  }
  /// Assign an element from double if possible
  virtual void fromDouble(size_t, double) {
    throw std::logic_error("Not implemented");
  }

protected:
  /// Sets the new column size.
  virtual void resize(size_t) { throw std::logic_error("Not implemented"); }
  /// Inserts an item.
  virtual void insert(size_t) { throw std::logic_error("Not implemented"); }
  /// Removes an item.
  virtual void remove(size_t) { throw std::logic_error("Not implemented"); }
  /// Pointer to a data element
  virtual void *void_pointer(size_t index) { return &m_data[index]; }
  /// Pointer to a data element
  virtual const void *void_pointer(size_t index) const {
    return &m_data[index];
  }
  /// Data storage
  std::vector<T> m_data;
};

} // anonymoius namespace

class FunctionDomainGeneralTest : public CxxTest::TestSuite {
public:
  static FunctionDomainGeneralTest *createSuite() {
    return new FunctionDomainGeneralTest();
  }
  static void destroySuite(FunctionDomainGeneralTest *suite) { delete suite; }

  void test_sizes() {
    Column_sptr column1(new TestColumn<int>(5));
    Column_sptr column2(new TestColumn<double>(5));
    Column_sptr column3(new TestColumn<std::string>(5));
    FunctionDomainGeneral domain;
    TS_ASSERT_THROWS_NOTHING(domain.addColumn(column1));
    TS_ASSERT_THROWS_NOTHING(domain.addColumn(column2));
    TS_ASSERT_THROWS_NOTHING(domain.addColumn(column3));
    TS_ASSERT_EQUALS(domain.size(), 5);
    TS_ASSERT_EQUALS(domain.columnCount(), 3);
    Column_sptr column4(new TestColumn<int>(2));
    TS_ASSERT_THROWS(domain.addColumn(column4), std::runtime_error);
  }

  void test_column_values() {
    Column_sptr column1(new TestColumn<int>(3));
    column1->cell<int>(0) = 11;
    column1->cell<int>(1) = 22;
    column1->cell<int>(2) = 33;
    Column_sptr column2(new TestColumn<std::string>(3));
    column2->cell<std::string>(0) = "Hello";
    column2->cell<std::string>(1) = "General";
    column2->cell<std::string>(2) = "Domain";
    FunctionDomainGeneral domain;
    TS_ASSERT_THROWS_NOTHING(domain.addColumn(column1));
    TS_ASSERT_THROWS_NOTHING(domain.addColumn(column2));
    auto intCol = domain.getColumn(0);
    TS_ASSERT_EQUALS(intCol->cell<int>(0), 11);
    TS_ASSERT_EQUALS(intCol->cell<int>(1), 22);
    TS_ASSERT_EQUALS(intCol->cell<int>(2), 33);
    auto strCol = domain.getColumn(1);
    TS_ASSERT_EQUALS(strCol->cell<std::string>(0), "Hello");
    TS_ASSERT_EQUALS(strCol->cell<std::string>(1), "General");
    TS_ASSERT_EQUALS(strCol->cell<std::string>(2), "Domain");
  }
};

#endif /*FUNCTIONDOMAINGENERALTEST_H_*/
