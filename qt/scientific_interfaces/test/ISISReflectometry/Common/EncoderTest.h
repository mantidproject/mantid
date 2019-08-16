// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef ISISREFLECTOMETRY_TEST_ENCODER_TEST_H_
#define ISISREFLECTOMETRY_TEST_ENCODER_TEST_H_

#include "../../../ISISReflectometry/GUI/Common/Encoder.h"
#include "../ReflMockObjects.h"
#include "CoderCommonTester.h"
#include "MantidAPI/FrameworkManager.h"

#include <cxxtest/TestSuite.h>

#include <QMap>
#include <QString>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class EncoderTest : public CxxTest::TestSuite {
public:
  static EncoderTest *createSuite() { return new EncoderTest(); }
  static void destroySuite(EncoderTest *suite) { delete suite; }

  void setUp() override { Mantid::API::FrameworkManager::Instance(); }

  void test_encoder() {
    CoderCommonTester tester;
    QtMainWindowView mwv;
    mwv.initLayout();
    Encoder encoder;
    auto map = encoder.encode(&mwv);

    map.insert(QString("tag"), QVariant(QString("ISIS Reflectometry")));

    tester.testMainWindowView(&mwv, map);
  }

  void test_encodeBatch() {
    CoderCommonTester tester;
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Encoder encoder;
    auto map = encoder.encodeBatch(gui, &mwv);

    tester.testBatch(gui, &mwv, map);
  }
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* ISISREFLECTOMETRY_TEST_ENCODER_TEST_H_ */