#ifndef VTKNULLSTRUCTUREDPOINTSARRAY_TEST_H_
#define VTKNULLSTRUCTUREDPOINTSARRAY_TEST_H_

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/vtkMDHWPointsArray.h"

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

class vtkMDHWPointsArrayTest : public CxxTest::TestSuite {
public:
  void testGetTuple() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWPointsArray<double>> spa;
    spa->InitializeArray(ws_sptr.get());

    long long dims[3];
    dims[0] = ws_sptr->getXDimension()->getNBins() + 1;
    dims[1] = ws_sptr->getYDimension()->getNBins() + 1;
    dims[2] = ws_sptr->getZDimension()->getNBins() + 1;

    auto index = 0;
    for (auto k = 0; k < dims[2]; ++k) {
      for (auto j = 0; j < dims[1]; ++j) {
        for (auto i = 0; i < dims[0]; ++i) {
          double output1[3];
          spa->GetTuple(index, output1);
          TS_ASSERT_DELTA(double(i), output1[0], 0.0001);
          TS_ASSERT_DELTA(double(j), output1[1], 0.0001);
          TS_ASSERT_DELTA(double(k), output1[2], 0.0001);

          // test alternate member function.
          double *output2 = spa->GetTuple(index);
          TS_ASSERT_DELTA(double(i), output2[0], 0.0001);
          TS_ASSERT_DELTA(double(j), output2[1], 0.0001);
          TS_ASSERT_DELTA(double(k), output2[2], 0.0001);

          // test alternate member function.
          double output3[3];
          spa->GetTupleValue(index, output3);
          TS_ASSERT_DELTA(double(i), output3[0], 0.0001);
          TS_ASSERT_DELTA(double(j), output3[1], 0.0001);
          TS_ASSERT_DELTA(double(k), output3[2], 0.0001);
          index++;
        }
      }
    }
  }

  void testGetValue() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWPointsArray<double>> spa;
    spa->InitializeArray(ws_sptr.get());

    long long dims[3];
    dims[0] = ws_sptr->getXDimension()->getNBins() + 1;
    dims[1] = ws_sptr->getYDimension()->getNBins() + 1;
    dims[2] = ws_sptr->getZDimension()->getNBins() + 1;

    auto index = 0;
    for (auto k = 0; k < dims[2]; ++k) {
      for (auto j = 0; j < dims[1]; ++j) {
        for (auto i = 0; i < dims[0]; ++i) {
          double value = spa->GetValue(index);
          TS_ASSERT_DELTA(double(i), value, 0.0001);
          index++;
          value = spa->GetValue(index);
          TS_ASSERT_DELTA(double(j), value, 0.0001);
          index++;
          value = spa->GetValue(index);
          TS_ASSERT_DELTA(double(k), value, 0.0001);
          index++;
        }
      }
    }

    index = 0;
    for (auto k = 0; k < dims[2]; ++k) {
      for (auto j = 0; j < dims[1]; ++j) {
        for (auto i = 0; i < dims[0]; ++i) {
          double value = spa->GetValueReference(index);
          TS_ASSERT_DELTA(double(i), value, 0.0001);
          index++;
          value = spa->GetValueReference(index);
          TS_ASSERT_DELTA(double(j), value, 0.0001);
          index++;
          value = spa->GetValueReference(index);
          TS_ASSERT_DELTA(double(k), value, 0.0001);
          index++;
        }
      }
    }

    index = 0;
    for (auto k = 0; k < dims[2]; ++k) {
      for (auto j = 0; j < dims[1]; ++j) {
        for (auto i = 0; i < dims[0]; ++i) {
          vtkVariant value = spa->GetVariantValue(index);
          TS_ASSERT_DELTA(double(i), value.ToDouble(), 0.0001);
          index++;
          value = spa->GetVariantValue(index);
          TS_ASSERT_DELTA(double(j), value.ToDouble(), 0.0001);
          index++;
          value = spa->GetVariantValue(index);
          TS_ASSERT_DELTA(double(k), value.ToDouble(), 0.0001);
          index++;
        }
      }
    }
  }

  void testGetTuplesPtIds() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWPointsArray<double>> spa;
    spa->InitializeArray(ws_sptr.get());

    long long dims[3];
    dims[0] = ws_sptr->getXDimension()->getNBins() + 1;
    dims[1] = ws_sptr->getYDimension()->getNBins() + 1;
    dims[2] = ws_sptr->getZDimension()->getNBins() + 1;

    vtkNew<vtkIdList> ptIds;

    for (auto idx = 0; idx < dims[0] * dims[1] * dims[2]; idx = idx + 4) {
      ptIds->InsertNextId(idx);
    }

    vtkNew<vtkDoubleArray> doubleArray;
    doubleArray->SetNumberOfComponents(3);
    doubleArray->Allocate(333);
    spa->GetTuples(ptIds.GetPointer(), doubleArray.GetPointer());

    for (auto idx = 0; idx < dims[0] * dims[1] * dims[2] / 4; ++idx) {
      double output1[3], output2[3];
      spa->GetTupleValue(idx * 4, output1);
      doubleArray->GetTupleValue(idx, output2);
      TS_ASSERT_DELTA(output1[0], output2[0], 0.0001);
      TS_ASSERT_DELTA(output1[1], output2[1], 0.0001);
      TS_ASSERT_DELTA(output1[2], output2[2], 0.0001);
    }
  }

  void testGetTuplesRange() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWPointsArray<double>> spa;
    spa->InitializeArray(ws_sptr.get());

    long long dims[3];
    dims[0] = ws_sptr->getXDimension()->getNBins() + 1;
    dims[1] = ws_sptr->getYDimension()->getNBins() + 1;
    dims[2] = ws_sptr->getZDimension()->getNBins() + 1;

    vtkNew<vtkDoubleArray> doubleArray;
    doubleArray->SetNumberOfComponents(3);
    doubleArray->Allocate(100);
    spa->GetTuples(0, 100, doubleArray.GetPointer());

    for (auto idx = 0; idx < 100; ++idx) {
      double output1[3], output2[3];
      spa->GetTupleValue(idx, output1);
      doubleArray->GetTupleValue(idx, output2);
      TS_ASSERT_DELTA(output1[0], output2[0], 0.0001);
      TS_ASSERT_DELTA(output1[1], output2[1], 0.0001);
      TS_ASSERT_DELTA(output1[2], output2[2], 0.0001);
    }
  }

  void testLookupOneValue() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWPointsArray<double>> spa;
    spa->InitializeArray(ws_sptr.get());
    TS_ASSERT(spa->LookupValue(1.0) == 3);
    TS_ASSERT(spa->LookupTypedValue(1.0) == 3);
  }

  void testLookupAllValues() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWPointsArray<double>> spa;
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
};

class vtkMDHWPointsArrayTestPerformance : public CxxTest::TestSuite {
public:
  long long dims[3];
  MDHistoWorkspace_sptr ws_sptr;
  vtkNew<vtkMDHWPointsArray<double>> m_spa;

  void setUp() {
    ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3, 200);
    m_spa->InitializeArray(ws_sptr.get());
    dims[0] = ws_sptr->getXDimension()->getNBins() + 1;
    dims[1] = ws_sptr->getYDimension()->getNBins() + 1;
    dims[2] = ws_sptr->getZDimension()->getNBins() + 1;
  }

  void tearDown() {}

  void testGetTupleValuePerformance() {
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
#endif
