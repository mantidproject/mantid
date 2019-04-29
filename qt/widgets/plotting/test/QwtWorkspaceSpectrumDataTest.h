// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_MANTIDQWTWORKSPACESPECTRUMDATA_H_
#define MANTIDQT_API_MANTIDQWTWORKSPACESPECTRUMDATA_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Plotting/Qwt/QwtWorkspaceSpectrumData.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <QRgb>
#include <cxxtest/TestSuite.h>
#include <limits>

class QwtWorkspaceSpectrumDataTest : public CxxTest::TestSuite {
  Mantid::API::MatrixWorkspace_sptr ws;

public:
  void setUp() override {
    ws = WorkspaceCreationHelper::create2DWorkspace(3, 4);
    for (size_t i = 0; i < 3; i++) {
      for (size_t j = 0; j < 5; j++)
        ws->dataX(i)[j] = double(i) + double(j);
      for (size_t j = 0; j < 4; j++) {
        ws->dataY(i)[j] = double(i) + double(j) * 2;
        ws->dataE(i)[j] = double(i) + double(j) * 3;
      }
    }
  }

  void checkHistogramData(QwtWorkspaceSpectrumData &data, double offset) {
    TS_ASSERT_EQUALS(data.size(), 4);
    for (size_t i = 0; i < 4; i++) {
      TS_ASSERT_DELTA(data.x(i), offset + double(i), 1e-5);
      TS_ASSERT_DELTA(data.y(i), offset + double(i) * 2, 1e-5);
      TS_ASSERT_DELTA(data.e(i), offset + double(i) * 3, 1e-5);
    }
    TS_ASSERT_DELTA(data.x(4), offset + double(4), 1e-5);
    // Go past the end in Y/E is safe
    TS_ASSERT_DELTA(data.y(4), offset + double(3) * 2, 1e-5);
  }

  void test_histogram() {
    QwtWorkspaceSpectrumData data(*ws, 1, false, false);
    checkHistogramData(data, 1.0);
    TS_ASSERT(data.isHistogram());
    QwtWorkspaceSpectrumData data2(*ws, 2, false, false);
    checkHistogramData(data2, 2.0);
  }

  void test_assigmentOperator() {
    QwtWorkspaceSpectrumData data1(*ws, 1, false, false);
    QwtWorkspaceSpectrumData data2 = data1;
    checkHistogramData(data2, 1.0);
  }

  void test_copy() {
    QwtWorkspaceSpectrumData data1(*ws, 1, false, false);
    QwtWorkspaceSpectrumData *data2 =
        dynamic_cast<QwtWorkspaceSpectrumData *>(data1.copy());
    checkHistogramData(*data2, 1.0);
  }

  /** In log scale, points below a minimum value are clipped to the minimum */
  void test_logScale() {
    ws->dataY(0)[2] = -10;
    QwtWorkspaceSpectrumData data(*ws, 0, true, false);
    TS_ASSERT_DELTA(data.y(1), 2.0, 1e-6);
    TS_ASSERT_DELTA(data.e(1), 3.0, 1e-6);
    TS_ASSERT_DELTA(data.y(2), 2.0, 1e-6);
    // Errors should also be zero-ed out.
    TS_ASSERT_DELTA(data.e(2), 0.0, 1e-6);
  }
};

#endif /* MANTIDQT_API_MANTIDQWTWORKSPACESPECTRUMDATA_H_ */
