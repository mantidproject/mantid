#ifndef MANTID_API_WORKSPACEPROPERTYWITHINDEXTEST_H_
#define MANTID_API_WORKSPACEPROPERTYWITHINDEXTEST_H_

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/FakeObjects.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceProperty.tcc"
#include "MantidAPI/WorkspacePropertyWithIndex.h"
#include "MantidAPI/WorkspacePropertyWithIndex.tcc"
#include "MantidKernel/PropertyManagerDataService.h"
#include <numeric>

using namespace Mantid::API;
using Mantid::API::WorkspacePropertyWithIndex;

using MatrixWorkspaceIndexProp = WorkspacePropertyWithIndex<MatrixWorkspace>;
class WorkspacePropertyWithIndexTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static WorkspacePropertyWithIndexTest *createSuite() {
    return new WorkspacePropertyWithIndexTest();
  }
  static void destroySuite(WorkspacePropertyWithIndexTest *suite) {
    delete suite;
  }

  void testConstructor() {
    TS_ASSERT_THROWS_NOTHING(
        (MatrixWorkspaceIndexProp(API::IndexType::SpectrumNumber)));
  }

  void testContructorFailsWithoutMatrixWorkspace() {
    // MatrixWorkspace needed for indexInfo.
    TS_ASSERT_THROWS(
        WorkspacePropertyWithIndex<Workspace>(
            static_cast<unsigned int>(API::IndexType::SpectrumNumber)),
        std::runtime_error);

    TS_ASSERT_THROWS(
        WorkspacePropertyWithIndex<ITableWorkspace>(
            static_cast<unsigned int>(API::IndexType::SpectrumNumber)),
        std::runtime_error);
  }

  void testConstructorFailsWithInvalidIndexType() {
    TS_ASSERT_THROWS(MatrixWorkspaceIndexProp(0), std::invalid_argument);
  }

  void testRetrieveWorkspaceAndSpectrumIndexSetUsingWorkspaceIndices() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);

    MatrixWorkspaceIndexProp prop(IndexType::WorkspaceIndex);
    prop = ws; // Set workspace pointer

    const auto &indexInfo = ws->indexInfo();

    // Create indices
    std::vector<int> indices(indexInfo.size());
    std::iota(indices.begin(), indices.end(), 0);

    prop.mutableIndexListProperty() = indices;

    MatrixWorkspace_sptr outWs;
    SpectrumIndexSet indexSet(indices.size());

    std::tie(outWs, indexSet) =
        std::tuple<MatrixWorkspace_sptr, SpectrumIndexSet>(prop);

    TS_ASSERT_EQUALS(outWs, ws);

    TS_ASSERT(std::equal(indexSet.begin(), indexSet.end(), indices.begin(),
                         indices.end()));
  }

  void testValidSpectrumNumbers() {
    // Spectrum Numbers will range from 1 to 10
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);

    MatrixWorkspaceIndexProp prop(IndexType::SpectrumNumber);
    prop = ws; // Set workspace pointer (necessary for using indexInfo)

    // Valid spectrum numbers could be set at the GUI level
    prop.mutableIndexListProperty() = std::vector<int>{1, 3, 8};

    TS_ASSERT(prop.isValid() == "");
  }

  void testInvalidSpectrumNumbers() {
    // Spectrum Numbers will range from 1 to 10
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);

    MatrixWorkspaceIndexProp prop(IndexType::SpectrumNumber);
    prop = ws; // Set workspace pointer

    // Invalid spectrum numbers could be set at the GUI level
    prop.mutableIndexListProperty() = std::vector<int>{25, 30, 95, 90};

    TS_ASSERT(prop.isValid() != "");
  }

  void testRetrieveWorkspaceAndSpectrumIndexSetUsingSpectrumNumbers() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);

    MatrixWorkspaceIndexProp prop(IndexType::SpectrumNumber);
    prop = ws; // Set workspace pointer

    const auto &indexInfo = ws->indexInfo();

    // Create indices
    std::vector<int> indices(indexInfo.size());
    for (size_t i = 0; i < indices.size(); i++)
      indices[i] = static_cast<int>(indexInfo.spectrumNumber(i));

    prop.mutableIndexListProperty() = indices;

    MatrixWorkspace_sptr outWs;
    SpectrumIndexSet indexSet(indices.size());

    std::tie(outWs, indexSet) =
        std::tuple<MatrixWorkspace_sptr, SpectrumIndexSet>(prop);

    TS_ASSERT_EQUALS(outWs, ws);

    for (int i = 0; i < indexSet.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i] + 1, indices[i]);
  }

  void testArbitrarySetOfSpectrumNumbers() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);

    MatrixWorkspaceIndexProp prop(IndexType::SpectrumNumber);
    prop = ws; // Set workspace pointer

    const auto &indexInfo = ws->indexInfo();

    // Create indices
    std::vector<int> indices{1, 3, 7, 5};

    prop.mutableIndexListProperty() = indices;

    MatrixWorkspace_sptr outWs;
    SpectrumIndexSet indexSet(indices.size());

    std::tie(outWs, indexSet) =
        std::tuple<MatrixWorkspace_sptr, SpectrumIndexSet>(prop);

    TS_ASSERT_EQUALS(outWs, ws);

    // Required because IndexInfo::makeIndexSet returns sorted indices.
    std::sort(indices.begin(), indices.end());

    for (int i = 0; i < indexSet.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i] + 1, indices[i]);
  }

  void testReturnAllIndicesWhenNoSpectrumNumbersProvided() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);

    MatrixWorkspaceIndexProp prop(IndexType::SpectrumNumber);
    prop = ws; // Set workspace pointer

    // Create indices
    MatrixWorkspace_sptr outWs;
    SpectrumIndexSet indexSet(0);

    std::tie(outWs, indexSet) =
        std::tuple<MatrixWorkspace_sptr, SpectrumIndexSet>(prop);

    TS_ASSERT_EQUALS(outWs, ws);

    TS_ASSERT_EQUALS(indexSet.size(), 10);

    for (int i = 0; i < indexSet.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i], i);
  }

  void testAssignWorkspaceTypeAndVector() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    std::vector<int> list{1, 3, 9};
    MatrixWorkspaceIndexProp prop(IndexType::SpectrumNumber);
    prop = std::tuple<MatrixWorkspace_sptr, std::string, std::vector<int>>(
        ws, "SpectrumNumber", list);

    MatrixWorkspace_sptr outWs;
    SpectrumIndexSet indexSet(0);

    std::tie(outWs, indexSet) =
        std::tuple<MatrixWorkspace_sptr, SpectrumIndexSet>(prop);

    TS_ASSERT_EQUALS(outWs, ws);

    TS_ASSERT_EQUALS(indexSet.size(), 3);

    for (int i = 0; i < list.size(); i++)
      TS_ASSERT_EQUALS(indexSet[i] + 1, list[i]);
  }

  void testAssignWorkspaceTypeAndString() {
    auto ws = WorkspaceFactory::Instance().create("WorkspaceTester", 10, 10, 9);
    std::string list("1:4,8");
    MatrixWorkspaceIndexProp prop(IndexType::WorkspaceIndex);
    prop = std::tuple<MatrixWorkspace_sptr, std::string, std::string>(
        ws, "WorkspaceIndex", list);

    MatrixWorkspace_sptr outWs;
    SpectrumIndexSet indexSet(0);

    std::tie(outWs, indexSet) =
        std::tuple<MatrixWorkspace_sptr, SpectrumIndexSet>(prop);

    TS_ASSERT_EQUALS(outWs, ws);

    TS_ASSERT_EQUALS(indexSet.size(), 5);
    TS_ASSERT_EQUALS(indexSet[0], 1);
    TS_ASSERT_EQUALS(indexSet[1], 2);
    TS_ASSERT_EQUALS(indexSet[2], 3);
    TS_ASSERT_EQUALS(indexSet[3], 4);
    TS_ASSERT_EQUALS(indexSet[4], 8);
  }
};

#endif /* MANTID_API_WORKSPACEPROPERTYWITHINDEXTEST_H_ */