#ifndef VTKNULLMDHWSIGNALARRAY_TEST_H_
#define VTKNULLMDHWSIGNALARRAY_TEST_H_

#include "MantidVatesAPI/Normalization.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/Normalization.h"
#include "MantidVatesAPI/vtkMDHWSignalArray.h"

#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "vtkIdList.h"
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>
#include <vtkDoubleArray.h>
#include <vtkPoints.h>
#include <iostream>

using namespace Mantid::VATES;
using namespace Mantid::DataObjects;

/*
 void InitializeArray(
 std::unique_ptr<Mantid::DataObjects::MDHistoWorkspaceIterator> iterator,
 std::size_t offset, vtkIdType size);

 // Reimplemented virtuals -- see superclasses for descriptions:
 void Initialize();
 void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output);
 void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);
 void Squeeze();
 vtkArrayIterator *NewIterator();
 vtkIdType LookupValue(vtkVariant value);
 void LookupValue(vtkVariant value, vtkIdList *ids);
 DONE: vtkVariant GetVariantValue(vtkIdType idx);
 NOT_IMPLEMENTED void ClearLookup();
 DONE: double *GetTuple(vtkIdType i);
 DONE: void GetTuple(vtkIdType i, double *tuple);
 vtkIdType LookupTypedValue(Scalar value);
 void LookupTypedValue(Scalar value, vtkIdList *ids);
 DONE: Scalar GetValue(vtkIdType idx);
 DONE: Scalar &GetValueReference(vtkIdType idx);
 DONE: void GetTupleValue(vtkIdType idx, Scalar *t);
 */

class vtkMDHWSignalArrayTest : public CxxTest::TestSuite {
public:
  void testGetTuple() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWSignalArray<double>> signal;
    std::size_t offset = 0;
    const int nBinsX = static_cast<int>(ws_sptr->getXDimension()->getNBins());
    const int nBinsY = static_cast<int>(ws_sptr->getYDimension()->getNBins());
    const int nBinsZ = static_cast<int>(ws_sptr->getZDimension()->getNBins());
    const int imageSize = (nBinsX) * (nBinsY) * (nBinsZ);

    std::unique_ptr<MDHistoWorkspaceIterator> iterator(
        dynamic_cast<MDHistoWorkspaceIterator *>(
            createIteratorWithNormalization(Mantid::VATES::NoNormalization,
                                            ws_sptr.get())));
    signal->InitializeArray(std::move(iterator), offset, imageSize);

    for (auto index = 0; index < imageSize; ++index) {
      double output1[1];
      signal->GetTuple(index, output1);
      TS_ASSERT_DELTA(1.0, output1[0], 0.0001);

      // test alternate member function.
      double *output2 = signal->GetTuple(index);
      TS_ASSERT_DELTA(1.0, output2[0], 0.0001);

      // test alternate member function.
      double output3[1];
      signal->GetTupleValue(index, output3);
      TS_ASSERT_DELTA(1.0, output3[0], 0.0001);

      // test alternate member function.
      TS_ASSERT_DELTA(1.0, signal->GetValue(index), 0.0001);

      // test alternate member function.
      TS_ASSERT_DELTA(1.0, signal->GetValueReference(index), 0.0001);

      // test alternate member function.
      vtkVariant value = signal->GetVariantValue(index);
      TS_ASSERT_DELTA(1.0, value.ToDouble(), 0.0001);
    }
  }

  void testGetTuplesPtIds() {

    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWSignalArray<double>> signal;
    std::size_t offset = 0;
    const int nBinsX = static_cast<int>(ws_sptr->getXDimension()->getNBins());
    const int nBinsY = static_cast<int>(ws_sptr->getYDimension()->getNBins());
    const int nBinsZ = static_cast<int>(ws_sptr->getZDimension()->getNBins());
    const int imageSize = (nBinsX) * (nBinsY) * (nBinsZ);

    std::unique_ptr<MDHistoWorkspaceIterator> iterator(
        dynamic_cast<MDHistoWorkspaceIterator *>(
            createIteratorWithNormalization(Mantid::VATES::NoNormalization,
                                            ws_sptr.get())));
    signal->InitializeArray(std::move(iterator), offset, imageSize);

    vtkNew<vtkIdList> ptIds;

    for (auto idx = 0; idx < imageSize; idx = idx + 4) {
      ptIds->InsertNextId(idx);
    }

    vtkNew<vtkDoubleArray> doubleArray;
    doubleArray->SetNumberOfComponents(1);
    doubleArray->Allocate(imageSize / 4);
    signal->GetTuples(ptIds.GetPointer(), doubleArray.GetPointer());

    for (auto idx = 0; idx < imageSize / 4; ++idx) {
      double output1[1], output2[1];
      signal->GetTupleValue(idx * 4, output1);
      doubleArray->GetTupleValue(idx, output2);
      TS_ASSERT_DELTA(output1[0], output2[0], 0.0001);
    }
  }

  void testGetTuplesRange() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWSignalArray<double>> signal;
    std::size_t offset = 0;
    const int nBinsX = static_cast<int>(ws_sptr->getXDimension()->getNBins());
    const int nBinsY = static_cast<int>(ws_sptr->getYDimension()->getNBins());
    const int nBinsZ = static_cast<int>(ws_sptr->getZDimension()->getNBins());
    const int imageSize = (nBinsX) * (nBinsY) * (nBinsZ);

    std::unique_ptr<MDHistoWorkspaceIterator> iterator(
        dynamic_cast<MDHistoWorkspaceIterator *>(
            createIteratorWithNormalization(Mantid::VATES::NoNormalization,
                                            ws_sptr.get())));
    signal->InitializeArray(std::move(iterator), offset, imageSize);

    vtkNew<vtkDoubleArray> doubleArray;
    doubleArray->SetNumberOfComponents(1);
    doubleArray->Allocate(100);
    signal->GetTuples(0, 100, doubleArray.GetPointer());

    for (auto idx = 0; idx < 100; ++idx) {
      double output1[1], output2[1];
      signal->GetTupleValue(idx, output1);
      doubleArray->GetTupleValue(idx, output2);
      TS_ASSERT_DELTA(output1[0], output2[0], 0.0001);
    }
  }
  /*
    void xtestLookupOneValue() {
      MDHistoWorkspace_sptr ws_sptr =
          MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
      vtkNew<vtkMDHWSignalArray<double>> spa;
      spa->InitializeArray(ws_sptr.get());
      TS_ASSERT(spa->LookupValue(1.0) == 3);
      TS_ASSERT(spa->LookupTypedValue(1.0) == 3);
    }

    void xtestLookupAllValues() {
      MDHistoWorkspace_sptr ws_sptr =
          MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
      vtkNew<vtkMDHWSignalArray<double>> spa;
      spa->InitializeArray(ws_sptr.get());
      vtkNew<vtkIdList> idList1, idList2;
      spa->LookupValue(0.0, idList1.GetPointer());
      for (vtkIdType i = 0; i < idList1->GetNumberOfIds(); ++i) {
        TS_ASSERT(spa->GetValue(idList1->GetId(i)) == 0.0);
      }

      spa->LookupTypedValue(1.0, idList2.GetPointer());
      for (vtkIdType i = 0; i < idList2->GetNumberOfIds(); ++i) {
        TS_ASSERT(spa->GetValue(idList2->GetId(i)) == 1.0);
      }
    }
    */
};

/*class vtkMDHWSignalArrayTestPerformance : public CxxTest::TestSuite {
public:
  long long dims[3];
  MDHistoWorkspace_sptr ws_sptr;
  vtkNew<vtkMDHWSignalArray<double>> m_spa;

  void setUp() {
    ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3, 200);
    m_spa->InitializeArray(ws_sptr.get());
    dims[0] = ws_sptr->getXDimension()->getNBins() + 1;
    dims[1] = ws_sptr->getYDimension()->getNBins() + 1;
    dims[2] = ws_sptr->getZDimension()->getNBins() + 1;
  }

  void tearDown() {}

  void xtestGetTupleValuePerformance() {
    auto index = 0;
    for (auto k = 0; k < dims[2]; ++k) {
      for (auto j = 0; j < dims[1]; ++j) {
        for (auto i = 0; i < dims[0]; ++i) {
          // test member function.
          double output[3];
          m_spa->GetTupleValue(index, output);
          TS_ASSERT_DELTA(0.05 * double(i), output[0], 0.0001);
          index++;
        }
      }
    }
  }
};
 */
#endif
