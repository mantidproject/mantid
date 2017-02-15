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

using namespace Mantid::VATES;
using namespace Mantid::DataObjects;

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
    signal->InitializeArray(ws_sptr.get(), Mantid::VATES::NoNormalization,
                            offset);

    for (auto index = 0; index < imageSize; ++index) {
      double output1[1];
      signal->GetTuple(index, output1);
      TS_ASSERT_DELTA(1.0, output1[0], 0.0001);

      // test alternate member function.
      double *output2 = signal->GetTuple(index);
      TS_ASSERT_DELTA(1.0, output2[0], 0.0001);

      // test alternate member function.
      double output3[1];
      signal->GetTypedTuple(index, output3);
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
            createIteratorWithNormalization(
                Mantid::VATES::NumEventsNormalization, ws_sptr.get())));
    signal->InitializeArray(ws_sptr.get(),
                            Mantid::VATES::NumEventsNormalization, offset);

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
      signal->GetTypedTuple(idx * 4, output1);
      doubleArray->GetTypedTuple(idx, output2);
      TS_ASSERT_DELTA(output1[0], output2[0], 0.0001);
    }
  }

  void testLookupMaskedValues() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3, 4);
    vtkNew<vtkMDHWSignalArray<double>> signal;
    std::size_t offset = 0;

    ws_sptr->setMDMaskAt(0, true);
    ws_sptr->setMDMaskAt(7, true);
    ws_sptr->setMDMaskAt(42, true);

    signal->InitializeArray(ws_sptr.get(), Mantid::VATES::NoNormalization,
                            offset);

    vtkNew<vtkIdList> idList1, idList2;

    signal->LookupTypedValue(1.0, idList2.GetPointer());
    TSM_ASSERT_EQUALS("IDs for the 61 unmasked points should have been found",
                      idList2->GetNumberOfIds(), 61);
  }

  void xtestGetTuplesRange() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWSignalArray<double>> signal;
    std::size_t offset = 0;
    signal->InitializeArray(ws_sptr.get(), Mantid::VATES::VolumeNormalization,
                            offset);

    vtkNew<vtkDoubleArray> doubleArray;
    doubleArray->SetNumberOfComponents(1);
    doubleArray->Allocate(100);
    signal->GetTuples(0, 99, doubleArray.GetPointer());

    for (auto idx = 0; idx < 100; ++idx) {
      double output1[1], output2[1];
      signal->GetTypedTuple(idx, output1);
      doubleArray->GetTypedTuple(idx, output2);
      TS_ASSERT_DELTA(output1[0], output2[0], 0.0001);
    }
  }

  void xtestLookupOneValue() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(8.0, 3, 10, 5.0);
    vtkNew<vtkMDHWSignalArray<double>> signal;
    std::size_t offset = 0;

    signal->InitializeArray(ws_sptr.get(), Mantid::VATES::NoNormalization,
                            offset);
    TS_ASSERT(signal->LookupValue(1.0) == 0);
    TS_ASSERT(signal->LookupTypedValue(1.0) == 0);
  }

  void xtestLookupAllValues() {
    MDHistoWorkspace_sptr ws_sptr =
        MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3);
    vtkNew<vtkMDHWSignalArray<double>> signal;
    std::size_t offset = 0;
    signal->InitializeArray(ws_sptr.get(), Mantid::VATES::NoNormalization,
                            offset);

    vtkNew<vtkIdList> idList1, idList2;
    signal->LookupValue(0.0, idList1.GetPointer());
    for (vtkIdType i = 0; i < idList1->GetNumberOfIds(); ++i) {
      TS_ASSERT(signal->GetValue(idList1->GetId(i)) == 1.0);
    }

    signal->LookupTypedValue(1.0, idList2.GetPointer());
    for (vtkIdType i = 0; i < idList2->GetNumberOfIds(); ++i) {
      TS_ASSERT(signal->GetValue(idList2->GetId(i)) == 1.0);
    }
  }
};

class vtkMDHWSignalArrayTestPerformance : public CxxTest::TestSuite {
public:
  MDHistoWorkspace_sptr ws_sptr;
  vtkNew<vtkMDHWSignalArray<double>> m_signal;
  int imageSize;

  void setUp() override {
    ws_sptr = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3, 200, 10.0,
                                                           1.0, "", 4.0);
    std::size_t offset = 0;
    const int nBinsX = static_cast<int>(ws_sptr->getXDimension()->getNBins());
    const int nBinsY = static_cast<int>(ws_sptr->getYDimension()->getNBins());
    const int nBinsZ = static_cast<int>(ws_sptr->getZDimension()->getNBins());
    imageSize = (nBinsX) * (nBinsY) * (nBinsZ);

    m_signal->InitializeArray(ws_sptr.get(),
                              Mantid::VATES::NumEventsNormalization, offset);
  }

  void tearDown() override {}

  void testGetTupleValuePerformance() {
    for (auto index = 0; index < imageSize; ++index) {
      // test member function.
      double output[1];
      m_signal->GetTypedTuple(index, output);
      TS_ASSERT_DELTA(0.25, output[0], 0.0001);
    }
  }
};

#endif
