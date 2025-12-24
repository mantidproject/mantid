// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Common/Encoder.h"
#include "../ReflMockObjects.h"
#include "CoderCommonTester.h"
#include "MantidPythonInterface/core/WrapPython.h"

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

  void setUp() override { Mantid::API::AlgorithmFactory::Instance().subscribe<ReflectometryISISLoadAndProcess>(); }

  void tearDown() override {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("ReflectometryISISLoadAndProcess", 1);
  }

  void test_encoder() {
    CoderCommonTester tester;
    QtMainWindowView mwv;
    mwv.initLayout();
    Encoder encoder;
    auto map = encoder.encode(&mwv, "");

    map.insert(QString("tag"), QVariant(QString("ISIS Reflectometry")));

    tester.testMainWindowView(&mwv, map);
  }

  void test_encodeBatch() {
    CoderCommonTester tester;
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Encoder encoder;
    auto map = encoder.encodeBatch(&mwv, 0);

    tester.testBatch(gui, &mwv, map);

    TS_ASSERT(map.contains(QString("version")))
    auto constexpr expectedVersion = "2";
    TS_ASSERT_EQUALS(expectedVersion, map[QString("version")].toString().toStdString())
  }

  void test_extractFromEncodingValidKey() {
    QVariantMap subItem{{"testsubkey", "testsubval"}};
    QVariantMap m{{"testkey", "testval"}, {"testkey1", subItem}, {"testkey2", "testval2"}};

    Encoder encoder;
    std::vector<std::string> key1{"testkey"};
    auto extract1 = encoder.extractFromEncoding(m, key1);

    std::vector<std::string> key2{"testkey1", "testsubkey"};
    auto extract2 = encoder.extractFromEncoding(m, key2);

    TS_ASSERT_EQUALS("testval", extract1)
    TS_ASSERT_EQUALS("testsubval", extract2)
  }

  void test_extractFromEncodingInvalidKey() {
    QVariantMap m{{"testkey", "testval"}};

    Encoder encoder;
    std::vector<std::string> key{"testkeyfalse"};
    TS_ASSERT_THROWS_EQUALS(encoder.extractFromEncoding(m, key), const std::invalid_argument &e, std::string(e.what()),
                            "Invalid json key provided. Json key not in map. Invalid element: testkeyfalse")
  }

  void test_extractFromEncodingInvalidPath() {
    QVariantMap m{{"testkey", "testval"}};

    Encoder encoder;
    std::vector<std::string> key{"testkey", "falsepath"};
    TS_ASSERT_THROWS_EQUALS(
        encoder.extractFromEncoding(m, key), const std::invalid_argument &e, std::string(e.what()),
        "Invalid json key provided. Json key must allow traversal of nested QMaps. Invalid element: falsepath")
  }
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
