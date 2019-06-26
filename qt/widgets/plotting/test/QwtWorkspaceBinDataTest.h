// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_MANTIDQWTWORKSPACEBINDATA_H_
#define MANTIDQT_API_MANTIDQWTWORKSPACEBINDATA_H_

#include "MantidQtWidgets/Plotting/Qwt/QwtWorkspaceBinData.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class QwtWorkspaceBinDataTest : public CxxTest::TestSuite {
  Mantid::API::MatrixWorkspace_sptr ws;

public:
  void setUp() override {
    ws = WorkspaceCreationHelper::create2DWorkspace(3, 4);
    auto ax1 = std::make_unique<Mantid::API::NumericAxis>(3);
    for (size_t i = 0; i < 3; i++) {
      ax1->setValue(i, 10.0 + static_cast<double>(i));
      for (size_t j = 0; j < 5; j++)
        ws->dataX(i)[j] = double(i) + double(j);
      for (size_t j = 0; j < 4; j++) {
        ws->dataY(i)[j] = double(i) + double(j) * 2;
        ws->dataE(i)[j] = double(i) + double(j) * 3;
      }
    }
    ws->replaceAxis(1, std::move(ax1));
  }

  void checkData(QwtWorkspaceBinData &data, double binIndex) {
    TS_ASSERT_EQUALS(data.size(), 3);
    for (size_t i = 0; i < 1; i++) {
      TS_ASSERT_DELTA(data.x(i), 10.0 + static_cast<double>(i), 1e-5);
      TS_ASSERT_DELTA(data.y(i), binIndex * 2.0 + double(i), 1e-5);
      TS_ASSERT_DELTA(data.e(i), binIndex * 3.0 + double(i), 1e-5);
    }
  }

  void test_data() {
    QwtWorkspaceBinData data(*ws, 1, false);
    checkData(data, 1.0);
    QwtWorkspaceBinData data2(*ws, 2, false);
    checkData(data2, 2.0);
  }

  void test_assigmentOperator() {
    QwtWorkspaceBinData data1(*ws, 1, false);
    QwtWorkspaceBinData data2 = data1;
    checkData(data2, 1.0);
  }

  void test_copy() {
    QwtWorkspaceBinData data1(*ws, 1, false);
    QwtWorkspaceBinData *data2 =
        dynamic_cast<QwtWorkspaceBinData *>(data1.copy());
    checkData(*data2, 1.0);
  }

  /** In log scale, points below a minimum value are clipped to the minimum */
  void test_logScale() {
    ws->dataY(2)[2] = -10;
    QwtWorkspaceBinData data(*ws, 2, true);
    TS_ASSERT_DELTA(data.y(1), 5.0, 1e-6);
    TS_ASSERT_DELTA(data.e(1), 7.0, 1e-6);
    TS_ASSERT_DELTA(data.y(2), 4.0, 1e-6);
    // Errors should also be zero-ed out.
    TS_ASSERT_DELTA(data.e(2), 0.0, 1e-6);
  }
};

#endif /* MANTIDQT_API_MANTIDQWTWORKSPACEBINDATA_H_ */
