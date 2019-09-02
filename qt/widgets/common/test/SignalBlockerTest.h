// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SIGNALBLOCKERTEST_H_
#define MANTID_API_SIGNALBLOCKERTEST_H_

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include <QObject>
#include <cxxtest/TestSuite.h>

using MantidQt::API::SignalBlocker;

class SignalBlockerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SignalBlockerTest *createSuite() { return new SignalBlockerTest(); }
  static void destroySuite(SignalBlockerTest *suite) { delete suite; }

  void test_obtain_and_release_behaviour() {
    QObject *toBlock = new QObject;
    {
      SignalBlocker scopedObj(toBlock);
      TSM_ASSERT("Should now block", toBlock->signalsBlocked());
    }
    TSM_ASSERT("Should no longer block", !toBlock->signalsBlocked());
    delete toBlock;
  }
};

#endif /* MANTID_API_SIGNALBLOCKERTEST_H_ */
