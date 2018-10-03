// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_SELECTION_NOTIFICATION_SERVICE_H_
#define MANTIDQT_API_SELECTION_NOTIFICATION_SERVICE_H_

#include "MantidQtWidgets/Common/SelectionNotificationService.h"
#include <cxxtest/TestSuite.h>

class SelectionNotificationServiceTest : public CxxTest::TestSuite {
public:
  /// check that we can get the singleton and call the send method, to emit a Qt
  /// signal
  void test_sendQSelection() {
    TS_ASSERT_THROWS_NOTHING(
        MantidQt::API::SelectionNotificationService::Instance()
            .sendQPointSelection(false, 1, 2, 3));
  }
};

#endif /* MANTIDQT_API_SELECTION_NOTIFICATION_SERVICE_H_ */
