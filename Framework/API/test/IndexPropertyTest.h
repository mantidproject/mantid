#ifndef MANTID_API_INDEXPROPERTYTEST_H_
#define MANTID_API_INDEXPROPERTYTEST_H_

#include "MantidAPI/IndexProperty.h"
#include "MantidAPI/IndexTypeProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/FakeObjects.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <numeric>

using Mantid::API::IndexType;
using Mantid::API::WorkspaceProperty;
using Mantid::API::IndexTypeProperty;
using Mantid::API::IndexProperty;

class IndexPropertyTest : public CxxTest::TestSuite {
public:
  IndexPropertyTest()
      : m_wkspProp("InputWorkspace", "", Kernel::Direction::Input) {}

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexPropertyTest *createSuite() { return new IndexPropertyTest(); }
  static void destroySuite(IndexPropertyTest *suite) { delete suite; }

  void testConstruct() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    m_wkspProp = ws;

    TS_ASSERT_THROWS_NOTHING(
        IndexProperty("IndexSet", m_wkspProp, m_itypeProp));
  }

  void testInvalidWorkspaceType() {
    WorkspaceProperty<Workspace> wkspProp("InputWorkspace", "",
                                          Kernel::Direction::Input);

    auto ws = boost::make_shared<TableWorkspaceTester>();
    wkspProp = ws;

    IndexProperty indexProp("IndexSet", wkspProp, m_itypeProp);

    TS_ASSERT_EQUALS(indexProp.isValid(),
                     "Invalid workspace type provided to "
                     "IndexProperty. Must be convertible to "
                     "MatrixWorkspace.")
  }

  void testSetIndicesUsingString() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    m_wkspProp = ws;

    IndexProperty indexProp("IndexSet", m_wkspProp, m_itypeProp);

    indexProp.setValue("0:4,7");

    auto indexSet = indexProp.getIndices();

    TS_ASSERT_EQUALS(indexSet.size(), 6);
    std::vector<int> testVec{0, 1, 2, 3, 4, 7};

    for (size_t i = 0; i < indexSet.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i], testVec[i]);
  }

  void testSetIndicesUsingVector() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    m_wkspProp = ws;

    IndexTypeProperty itypeProp("IndexType", IndexType::SpectrumNum);
    IndexProperty indexProp("IndexSet", m_wkspProp, itypeProp);
    std::vector<int> input{1, 3, 5, 7};
    indexProp = input;

    auto indexSet = indexProp.getIndices();

    TS_ASSERT_EQUALS(indexSet.size(), 4);

    for (size_t i = 0; i < indexSet.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i], input[i] - 1);
  }

  void testInvalidWhenIndicesOutOfRange() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    m_wkspProp = ws;

    IndexProperty indexProp("IndexSet", m_wkspProp, m_itypeProp);

    indexProp.setValue("30:35");

    TS_ASSERT_EQUALS(indexProp.isValid(),
                     "Indices provided to IndexProperty are out of range.");
  }

  void testRetrieveShortStringValueForPureRangeVector() {
    auto ws =
        WorkspaceFactory::Instance().create("WorkspaceTester", 1000, 10, 9);
    m_wkspProp = ws;

    IndexProperty indexProp("IndexSet", m_wkspProp, m_itypeProp);

    std::vector<int> input(1000);
    std::iota(input.begin(), input.end(), 0);
    indexProp = input;

    TS_ASSERT_EQUALS(indexProp.value(), "0:999");
    TS_ASSERT_EQUALS(indexProp.isValid(), "");
  }

  void testRetrieveShortStringValueForMinMaxString() {
    auto ws =
        WorkspaceFactory::Instance().create("WorkspaceTester", 100, 10, 9);
    m_wkspProp = ws;

    IndexProperty indexProp("IndexSet", m_wkspProp, m_itypeProp);

    indexProp.setValue("10:55");

    TS_ASSERT_EQUALS(indexProp.value(), "10:55");
    TS_ASSERT_EQUALS(indexProp.isValid(), "");
  }

  void testRetrieveShortStringValueForPureRangeString() {
    auto ws =
        WorkspaceFactory::Instance().create("WorkspaceTester", 100, 10, 9);
    m_wkspProp = ws;

    IndexProperty indexProp("IndexSet", m_wkspProp, m_itypeProp);

    indexProp.setValue(
        "10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30");

    TS_ASSERT_EQUALS(indexProp.value(), "10:30");
    TS_ASSERT_EQUALS(indexProp.isValid(), "");
  }

private:
  WorkspaceProperty<MatrixWorkspace> m_wkspProp;
  IndexTypeProperty m_itypeProp;
};

#endif /* MANTID_API_INDEXPROPERTYTEST_H_ */