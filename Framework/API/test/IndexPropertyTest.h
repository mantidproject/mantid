// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_INDEXPROPERTYTEST_H_
#define MANTID_API_INDEXPROPERTYTEST_H_

#include "MantidAPI/IndexProperty.h"
#include "MantidAPI/IndexTypeProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/PropertyManager.h"

#include "MantidTestHelpers/FakeObjects.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::API::IndexType;
using Mantid::Kernel::Direction;

class IndexPropertyTest : public CxxTest::TestSuite {
public:
  IndexPropertyTest() : m_wkspProp("InputWorkspace", "", Direction::Input) {}

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
                                          Direction::Input);

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
    std::vector<int64_t> testVec{0, 1, 2, 3, 4, 7};

    for (size_t i = 0; i < indexSet.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i], testVec[i]);
  }

  void testAssignIndicesUsingString() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    m_wkspProp = ws;

    IndexProperty indexProp("IndexSet", m_wkspProp, m_itypeProp);

    indexProp = "0-5";

    auto indexSet = indexProp.getIndices();

    TS_ASSERT_EQUALS(indexSet.size(), 6);
    std::vector<int64_t> testVec{0, 1, 2, 3, 4, 5};

    for (size_t i = 0; i < indexSet.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i], testVec[i]);
  }

  void testAssignIndicesUsingVector() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    m_wkspProp = ws;

    IndexTypeProperty itypeProp("IndexType", IndexType::SpectrumNum);
    IndexProperty indexProp("IndexSet", m_wkspProp, itypeProp);
    std::vector<int64_t> input{1, 3, 5, 7};
    indexProp = input;

    auto indexSet = indexProp.getIndices();

    TS_ASSERT_EQUALS(indexSet.size(), 4);

    for (size_t i = 0; i < indexSet.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i], input[i] - 1);
  }

  void testIndexOrderOfFullRangePreserved() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 3, 1, 1);
    m_wkspProp = ws;
    IndexTypeProperty itypeProp("IndexType", IndexType::WorkspaceIndex);
    IndexProperty indexProp("IndexSet", m_wkspProp, itypeProp);
    std::vector<int64_t> input{0, 2, 1};
    indexProp = input;

    auto indexSet = indexProp.getIndices();

    TS_ASSERT_EQUALS(indexSet.size(), 3);
    TS_ASSERT_EQUALS(indexSet[0], 0);
    TS_ASSERT_EQUALS(indexSet[1], 2);
    TS_ASSERT_EQUALS(indexSet[2], 1);
  }

  void testInvalidWhenIndicesOutOfRange() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    m_wkspProp = ws;

    IndexProperty indexProp("IndexSet", m_wkspProp, m_itypeProp);

    auto error = indexProp.setValue("30:35");

    TS_ASSERT(
        error.find("Indices provided to IndexProperty are out of range.") !=
        std::string::npos);
  }

  void testIndexAccessWithOperator() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    m_wkspProp = ws;

    IndexProperty indexProp("IndexSet", m_wkspProp, m_itypeProp);

    indexProp.setValue("1:5");

    auto indices = Mantid::Indexing::SpectrumIndexSet(indexProp);

    TS_ASSERT(indices.size() == 5);
    for (int64_t i = 0; i < 5; i++)
      TS_ASSERT_EQUALS(indices[i], i + 1)
  }

  void testGeneratePropertyName() {
    std::string propName = "InputWorkspace";
    TS_ASSERT_EQUALS(propName + "IndexSet",
                     IndexProperty::generatePropertyName(propName));
  }

  void testGetFilteredIndexInfo_WorkspaceIndex() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 3, 1, 1);
    m_wkspProp = ws;
    IndexTypeProperty itypeProp("IndexType", IndexType::WorkspaceIndex);
    IndexProperty indexProp("IndexSet", m_wkspProp, itypeProp);

    auto indexInfo = indexProp.getFilteredIndexInfo();
    TS_ASSERT_EQUALS(indexInfo.size(), 3);

    std::vector<int64_t> input{1, 2};
    indexProp = input;
    indexInfo = indexProp.getFilteredIndexInfo();
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), 3);
  }

  void testGetFilteredIndexInfo_SpectrumNum() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 3, 1, 1);
    m_wkspProp = ws;
    IndexTypeProperty itypeProp("IndexType", IndexType::SpectrumNum);
    IndexProperty indexProp("IndexSet", m_wkspProp, itypeProp);

    auto indexInfo = indexProp.getFilteredIndexInfo();
    TS_ASSERT_EQUALS(indexInfo.size(), 3);

    std::vector<int64_t> input{1, 2};
    indexProp = input;
    indexInfo = indexProp.getFilteredIndexInfo();
    TS_ASSERT_EQUALS(indexInfo.size(), 2);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(indexInfo.spectrumNumber(1), 2);
  }

private:
  WorkspaceProperty<MatrixWorkspace> m_wkspProp;
  IndexTypeProperty m_itypeProp;
};

#endif /* MANTID_API_INDEXPROPERTYTEST_H_ */
