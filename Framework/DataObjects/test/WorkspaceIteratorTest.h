// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <memory>

#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MantidVec;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

template <typename T> class FibSeries {
private:
  T x1; /// Initial value 1;
  T x2; /// Initial value 2;

public:
  FibSeries() : x1(1), x2(1) {}
  T operator()() {
    const T out(x1 + x2);
    x1 = x2;
    x2 = out;
    return out;
  }
};

class WorkspaceIteratorTest : public CxxTest::TestSuite {
private:
  using parray = std::shared_ptr<MantidVec>;
  using W2D = std::shared_ptr<Workspace2D>;
  using WSV = std::shared_ptr<WorkspaceSingleValue>;
  using Wbase = std::shared_ptr<MatrixWorkspace>;

public:
  void testIteratorWorkspace2DAsBase() {
    int size = 57;
    int histogramCount = 100;
    Wbase workspace = WorkspaceCreationHelper::Create2DWorkspace(histogramCount, size);
    // workspace->dataX(0) // this is the first spectrum in the workspace with
    // real data
    int count = 0;
    for (MatrixWorkspace::const_iterator ti(*workspace); ti != ti.end(); ++ti) {
      TS_ASSERT_THROWS_NOTHING(LocatedDataRef tr = *ti; int datablock = count / size; int blockindex = count % size;
                               TS_ASSERT_EQUALS(tr.X(), workspace->dataX(datablock)[blockindex]);
                               TS_ASSERT_EQUALS(tr.Y(), workspace->dataY(datablock)[blockindex]);
                               TS_ASSERT_EQUALS(tr.E(), workspace->dataE(datablock)[blockindex]);)
      count++;
    }
    TS_ASSERT_EQUALS(count, size * histogramCount);
  }

  void testHorizontalLoopIteratorWorkspace2D() {
    int size = 57;
    int histogramCount = 100;
    Wbase workspace = WorkspaceCreationHelper::Create2DWorkspace(histogramCount, size);

    const int loopCountArrayLength = 4;
    int loopCountArray[loopCountArrayLength];
    loopCountArray[0] = 1;
    loopCountArray[1] = 2;
    loopCountArray[2] = 3;
    loopCountArray[3] = 0;

    for (int i = 0; i < loopCountArrayLength; i++) {
      int loopCount = loopCountArray[i];
      int count = 0;
      for (MatrixWorkspace::const_iterator ti(*workspace, loopCount); ti != ti.end(); ++ti) {
        TS_ASSERT_THROWS_NOTHING(LocatedDataRef tr = *ti; int indexPosition = count % (size * histogramCount);
                                 int datablock = indexPosition / size; int blockindex = indexPosition % size;
                                 TS_ASSERT_EQUALS(tr.X(), workspace->dataX(datablock)[blockindex]);
                                 TS_ASSERT_EQUALS(tr.Y(), workspace->dataY(datablock)[blockindex]);
                                 TS_ASSERT_EQUALS(tr.E(), workspace->dataE(datablock)[blockindex]);)
        count++;
      }
      TS_ASSERT_EQUALS(count, size * histogramCount * loopCount);
    }
  }

  void testVerticalLoopIteratorWorkspace2D() {
    int size = 50;
    int histogramCount = 100;
    Wbase workspace = WorkspaceCreationHelper::Create2DWorkspace(histogramCount, size);

    const int loopCountArrayLength = 4;
    int loopCountArray[loopCountArrayLength];
    loopCountArray[0] = 1;
    loopCountArray[1] = 2;
    loopCountArray[2] = 3;
    loopCountArray[3] = 0;

    for (int i = 0; i < loopCountArrayLength; i++) {
      int loopCount = loopCountArray[i];
      int count = 0;
      for (MatrixWorkspace::const_iterator ti(*workspace, loopCount, LoopOrientation::Vertical); ti != ti.end(); ++ti) {
        // TS_ASSERT_THROWS_NOTHING
        //(
        //  //LocatedDataRef tr = *ti;
        //  //int datablock = count/(size*loopCount);
        //  //int blockindex = count/loopCount;
        //  //TS_ASSERT_EQUALS(tr.X(),workspace->dataX(datablock)[blockindex]);
        //  //TS_ASSERT_EQUALS(tr.Y(),workspace->dataY(datablock)[blockindex]);
        //  //TS_ASSERT_EQUALS(tr.E(),workspace->dataE(datablock)[blockindex]);
        //)
        count++;
      }
      TS_ASSERT_EQUALS(count, size * histogramCount * loopCount);
    }
  }

  void testIteratorWorkspaceSingleValueLength() {
    Wbase workspace = WSV(new WorkspaceSingleValue(1.1, 2.2));

    int count = 0;
    for (MatrixWorkspace::const_iterator ti(*workspace); ti != ti.end(); ++ti) {
      TS_ASSERT_THROWS_NOTHING(LocatedDataRef tr = *ti; TS_ASSERT_EQUALS(tr.X(), workspace->dataX(0)[count]);
                               TS_ASSERT_EQUALS(tr.Y(), workspace->dataY(0)[count]);
                               TS_ASSERT_EQUALS(tr.E(), workspace->dataE(0)[count]);)
      count++;
    }
    TS_ASSERT_EQUALS(count, 1);
  }

  void testHorizontalLoopIteratorWorkspaceSingleValue() {
    int size = 1;
    int histogramCount = 1;
    Wbase workspace = WSV(new WorkspaceSingleValue(1.4, 2.4));

    const int loopCountArrayLength = 4;
    int loopCountArray[loopCountArrayLength];
    loopCountArray[0] = 1;
    loopCountArray[1] = 50;
    loopCountArray[2] = 200;
    loopCountArray[3] = 0;

    for (int i = 0; i < loopCountArrayLength; i++) {
      int loopCount = loopCountArray[i];
      int count = 0;
      for (MatrixWorkspace::const_iterator ti(*workspace, loopCount); ti != ti.end(); ++ti) {
        TS_ASSERT_THROWS_NOTHING(LocatedDataRef tr = *ti; int indexPosition = count % (size * histogramCount);
                                 int datablock = indexPosition / size; int blockindex = indexPosition % size;
                                 TS_ASSERT_EQUALS(tr.X(), workspace->dataX(datablock)[blockindex]);
                                 TS_ASSERT_EQUALS(tr.Y(), workspace->dataY(datablock)[blockindex]);
                                 TS_ASSERT_EQUALS(tr.E(), workspace->dataE(datablock)[blockindex]);)
        count++;
      }
      TS_ASSERT_EQUALS(count, size * histogramCount * loopCount);
    }
  }
};
