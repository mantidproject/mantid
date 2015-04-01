#ifndef MANTID_API_PROJECTIONTEST_H_
#define MANTID_API_PROJECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Projection.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid;
using namespace Mantid::API;

namespace {
// Provides a table that claims to have the given number of rows and columns.
class DimensionedTable : public TableWorkspaceTester {
public:
  DimensionedTable(size_t cols, size_t rows)
      : m_numColumns(cols), m_numRows(rows) {}
  size_t columnCount() const { return m_numColumns; }
  size_t rowCount() const { return m_numRows; }
private:
  size_t m_numColumns;
  size_t m_numRows;
};

// Provides an example table that's properly formatted

class NameColumn : public ColumnTester {
public:
  NameColumn() {
    m_names[0] = "u";
    m_names[1] = "v";
    m_names[2] = "w";
  }
  size_t size() const { return 3; }

  using ColumnTester::void_pointer;
  const void* void_pointer(size_t index) const {
    return &m_names[index];
  }
private:
  std::string m_names[3];
};

class ValueColumn : public ColumnTester {
public:
  ValueColumn() {
    m_values[0] = "1,1,0";
    m_values[1] = "-1,1,0";
    m_values[2] = "0,0,1";
  }
  size_t size() const { return 3; }

  using ColumnTester::void_pointer;
  const void* void_pointer(size_t index) const {
    return &m_values[index];
  }
private:
  std::string m_values[3];
};

class OffsetColumn : public ColumnTester {
public:
  OffsetColumn() {
    m_offsets[0] = 0.5;
    m_offsets[1] = 1.25;
    m_offsets[2] = -10.0;
  }
  size_t size() const { return 3; }

  using ColumnTester::void_pointer;
  const void* void_pointer(size_t index) const {
    return &m_offsets[index];
  }
private:
  double m_offsets[3];
};

class UnitColumn : public ColumnTester {
public:
  UnitColumn() {
    m_units[0] = "r";
    m_units[1] = "a";
    m_units[2] = "r";
  }
  size_t size() const { return 3; }

  using ColumnTester::void_pointer;
  const void* void_pointer(size_t index) const {
    return &m_units[index];
  }
private:
  std::string m_units[3];
};

class GoodTable : public TableWorkspaceTester {
  size_t columnCount() const { return 4; }
  size_t rowCount() const { return 3; }

  using TableWorkspaceTester::getColumn;
  Column_const_sptr getColumn(const std::string& name) const {
    if (name == "name")
      return Column_const_sptr(new NameColumn());
    else if (name == "value")
      return Column_const_sptr(new ValueColumn());
    else if (name == "offset")
      return Column_const_sptr(new OffsetColumn());
    else if (name == "type")
      return Column_const_sptr(new UnitColumn());
    else
      throw std::runtime_error("unknown column: " + name);
  }
};
}

class ProjectionTest : public CxxTest::TestSuite {
public:
  void test_blank_constructor() {
    Projection p;
    TS_ASSERT_EQUALS(p.getOffset(0), 0.0);
    TS_ASSERT_EQUALS(p.U(), V3D(1,0,0));
    TS_ASSERT_EQUALS(p.V(), V3D(0,1,0));
    TS_ASSERT_EQUALS(p.W(), V3D(0,0,1));
    TS_ASSERT_EQUALS(p.getUnit(0), RLU);
    TS_ASSERT_EQUALS(p.getUnit(1), RLU);
    TS_ASSERT_EQUALS(p.getUnit(2), RLU);
  }

  void test_uvw_constructors() {
    V3D u(1, -1, 0);
    V3D v(1, 1, 0);
    V3D w(0, 0, 1);
    Projection p(u, v, w);

    TS_ASSERT_EQUALS(p.U(), u);
    TS_ASSERT_EQUALS(p.V(), v);
    TS_ASSERT_EQUALS(p.W(), w);
  }

  void test_construct_null_workspace() {
    try {
      auto p = boost::make_shared<Projection>(ITableWorkspace_sptr());
      TS_FAIL("Projection constructor should have thrown exception");
    } catch(std::runtime_error& e) {
      TS_ASSERT_EQUALS(e.what(),
          std::string("Null ITableWorkspace given to Projection constructor"))
    } catch(...) {
      TS_FAIL("Projection constructor threw unexpected exception");
    }
  }

  void test_construct_bad_workspace_columns() {
    auto proj = ITableWorkspace_sptr(new DimensionedTable(0,0));
    try {
      auto p = boost::make_shared<Projection>(proj);
      TS_FAIL("Projection constructor should have thrown exception");
    } catch(std::runtime_error& e) {
      TS_ASSERT_EQUALS(e.what(),
          std::string("4 columns must be provided to create a projection"))
    } catch(...) {
      TS_FAIL("Projection constructor threw unexpected exception");
    }
  }

  void test_construct_bad_workspace_no_rows() {
    auto proj = ITableWorkspace_sptr(new DimensionedTable(4,0));
    try {
      auto p = boost::make_shared<Projection>(proj);
      TS_FAIL("Projection constructor should have thrown exception");
    } catch(std::runtime_error& e) {
      TS_ASSERT_EQUALS(e.what(),
          std::string("3 rows must be provided to create a projection"))
    } catch(...) {
      TS_FAIL("Projection constructor threw unexpected exception");
    }
  }

  void test_construct_bad_workspace_too_many_rows() {
    auto proj = ITableWorkspace_sptr(new DimensionedTable(4,4));
    try {
      auto p = boost::make_shared<Projection>(proj);
      TS_FAIL("Projection constructor should have thrown exception");
    } catch(std::runtime_error& e) {
      TS_ASSERT_EQUALS(e.what(),
          std::string("3 rows must be provided to create a projection"))
    } catch(...) {
      TS_FAIL("Projection constructor threw unexpected exception");
    }
  }

  void test_construct_good_workspace() {
    auto proj = ITableWorkspace_sptr(new GoodTable());
    Projection_sptr p;
    TS_ASSERT_THROWS_NOTHING(p = boost::make_shared<Projection>(proj));

    TS_ASSERT_EQUALS(p->U(), V3D(1, 1, 0));
    TS_ASSERT_EQUALS(p->V(), V3D(-1, 1, 0));
    TS_ASSERT_EQUALS(p->W(), V3D(0, 0, 1));
    TS_ASSERT_EQUALS(p->getOffset(0), 0.5);
    TS_ASSERT_EQUALS(p->getOffset(1), 1.25);
    TS_ASSERT_EQUALS(p->getOffset(2), -10.0);
    TS_ASSERT_EQUALS(p->getUnit(0), RLU);
    TS_ASSERT_EQUALS(p->getUnit(1), INV_ANG);
    TS_ASSERT_EQUALS(p->getUnit(2), RLU);
  }

  void test_throw_out_of_range_access() {
    Projection p;
    TS_ASSERT_THROWS_ANYTHING(p.getOffset(-1));
    TS_ASSERT_THROWS_NOTHING(p.getOffset(2));
    TS_ASSERT_THROWS_ANYTHING(p.getOffset(3));

    TS_ASSERT_THROWS_ANYTHING(p.getAxis(-1));
    TS_ASSERT_THROWS_NOTHING(p.getAxis(2));
    TS_ASSERT_THROWS_ANYTHING(p.getAxis(3));

    TS_ASSERT_THROWS_ANYTHING(p.getUnit(-1));
    TS_ASSERT_THROWS_NOTHING(p.getUnit(2));
    TS_ASSERT_THROWS_ANYTHING(p.getUnit(3));
  }

  void test_copy_constructor() {
    V3D u(1, -1, 0);
    V3D v(1, 1, 0);
    V3D w(0, 0, 1);
    Projection p(u, v, w);
    p.setUnit(0, RLU);
    p.setUnit(1, INV_ANG);

    Projection q(p);

    TS_ASSERT_EQUALS(q.getAxis(0), u);
    TS_ASSERT_EQUALS(q.getAxis(1), v);
    TS_ASSERT_EQUALS(q.getAxis(2), w);
    TS_ASSERT_EQUALS(q.getUnit(0), RLU);
    TS_ASSERT_EQUALS(q.getUnit(1), INV_ANG);
  }

  void test_assign() {
    V3D u(1, -1, 0);
    V3D v(1, 1, 0);
    V3D w(0, 0, 1);
    Projection p(u, v, w);

    Projection q;

    q = p;

    TS_ASSERT_EQUALS(q.getAxis(0), u);
    TS_ASSERT_EQUALS(q.getAxis(1), v);
    TS_ASSERT_EQUALS(q.getAxis(2), w);
  }

  void test_setOffset() {
    Projection p;
    p.setOffset(0, 1.00);
    p.setOffset(1, 1.50);
    p.setOffset(2, 4.75);
    TS_ASSERT_EQUALS(p.getOffset(0), 1.00);
    TS_ASSERT_EQUALS(p.getOffset(1), 1.50);
    TS_ASSERT_EQUALS(p.getOffset(2), 4.75);
  }

  void test_setAxis() {
    Projection p;
    p.setAxis(0, V3D(1,2,3));
    p.setAxis(1, V3D(4,5,6));
    p.setAxis(2, V3D(7,8,8));
    TS_ASSERT_EQUALS(p.getAxis(0), V3D(1,2,3));
    TS_ASSERT_EQUALS(p.getAxis(1), V3D(4,5,6));
    TS_ASSERT_EQUALS(p.getAxis(2), V3D(7,8,8));
  }

  void test_setUnit() {
    Projection p;
    p.setUnit(0, INV_ANG);
    p.setUnit(1, RLU);
    p.setUnit(2, INV_ANG);
    TS_ASSERT_EQUALS(p.getUnit(0), INV_ANG);
    TS_ASSERT_EQUALS(p.getUnit(1), RLU);
    TS_ASSERT_EQUALS(p.getUnit(2), INV_ANG);
  }
};

#endif /* MANTID_API_PROJECTIONTEST_H_ */
