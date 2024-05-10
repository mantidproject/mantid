// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Common/Decoder.h"
#include "../ReflMockObjects.h"
#include "CoderCommonTester.h"
#include "MantidAPI/FileFinder.h"
#include "MantidPythonInterface/core/WrapPython.h"
#include "MantidQtWidgets/Common/QtJSONUtils.h"

#include <cxxtest/TestSuite.h>

#include <QMap>
#include <QString>
#include <QVariant>

namespace {

const std::string DIR_PATH = "ISISReflectometry/";
auto &fileFinder = FileFinder::Instance();
const auto MAINWINDOW_FILE = fileFinder.getFullPath(DIR_PATH + "mainwindow.json");
const auto BATCH_FILE_PREVIOUS = fileFinder.getFullPath(DIR_PATH + "batch_previous.json");
const auto BATCH_FILE_V1 = fileFinder.getFullPath(DIR_PATH + "batch.json");
const auto BATCH_FILE_POLREF = fileFinder.getFullPath(DIR_PATH + "batch_POLREF.json");
const auto EMPTY_BATCH_FILE = fileFinder.getFullPath(DIR_PATH + "empty_batch.json");
const auto TWO_ROW_EXP_BATCH_FILE = fileFinder.getFullPath(DIR_PATH + "batch_2_exp_rows.json");
const auto EIGHT_COL_BATCH_FILE = fileFinder.getFullPath(DIR_PATH + "8_col_batch.json");
const auto NINE_COL_BATCH_FILE = fileFinder.getFullPath(DIR_PATH + "9_col_batch.json");
const auto TEN_COL_BATCH_FILE = fileFinder.getFullPath(DIR_PATH + "10_col_batch.json");
const auto ELEVEN_COL_BATCH_FILE = fileFinder.getFullPath(DIR_PATH + "11_col_batch.json");

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class DecoderTest : public CxxTest::TestSuite {
public:
  static DecoderTest *createSuite() { return new DecoderTest(); }
  static void destroySuite(DecoderTest *suite) { delete suite; }

  void setUp() override { Mantid::API::AlgorithmFactory::Instance().subscribe<ReflectometryISISLoadAndProcess>(); }

  void tearDown() override {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("ReflectometryISISLoadAndProcess", 1);
  }

  void test_decodeMainWindow() {
    CoderCommonTester tester;
    Decoder decoder;
    auto map = MantidQt::API::loadJSONFromFile(QString::fromStdString(MAINWINDOW_FILE));
    auto widget = decoder.decode(map, "");
    tester.testMainWindowView(dynamic_cast<QtMainWindowView *>(widget), map);
  }

  void test_decodeEmptyBatch() {
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONFromFile(QString::fromStdString(EMPTY_BATCH_FILE));
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    decoder.decodeBatch(&mwv, 0, map);

    tester.testBatch(gui, &mwv, map);
  }

  void test_decodePopulatedBatch() {
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONFromFile(QString::fromStdString(BATCH_FILE_V1));
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    decoder.decodeBatch(&mwv, 0, map);

    tester.testBatch(gui, &mwv, map);
  }

  void test_decodePopulatedPOLREFBatch() {
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONFromFile(QString::fromStdString(BATCH_FILE_POLREF));
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    decoder.decodeBatch(&mwv, 0, map);

    tester.testBatch(gui, &mwv, map);
  }

  void test_decodeOldPopulatedBatchFile() {
    // Check we maintain backwards compatibility when controls are added or changed
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONFromFile(QString::fromStdString(BATCH_FILE_PREVIOUS));
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    decoder.decodeBatch(&mwv, 0, map);

    tester.testBatch(gui, &mwv, map);
  }

  void test_decodePopulatedBatchWithPopulatedGUI() {
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONFromFile(QString::fromStdString(TWO_ROW_EXP_BATCH_FILE));
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    decoder.decodeBatch(&mwv, 0, map);
    decoder.decodeBatch(&mwv, 0, map);

    tester.testBatch(gui, &mwv, map);
  }

  void test_decodeBatchWhenInstrumentChanged() {
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONFromFile(QString::fromStdString(BATCH_FILE_V1));
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    // Set the initial instrument to something different to the one we are
    // decoding
    gui->runs()->setSearchInstrument("POLREF");

    Decoder decoder;
    decoder.decodeBatch(&mwv, 0, map);

    tester.testBatch(gui, &mwv, map);
  }

  void test_decodeLegacyElevenColBatchFile() {
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    // Decode from the old 9-column format
    auto oldMap = MantidQt::API::loadJSONFromFile(QString::fromStdString(ELEVEN_COL_BATCH_FILE));
    decoder.decodeBatch(&mwv, 0, oldMap);

    // Check that the result matches the new format
    QList<QVariant> expectedRowValues{"0.5", ".*", "13463", "13464", "4", "0.01", "0.1", "0.02", "", "4", "5", ""};
    CoderCommonTester tester;
    constexpr auto rowIndex = int{0};
    tester.checkPerAngleDefaultsRowEquals(gui, expectedRowValues, rowIndex);
  }

  void test_decodeLegacyTenColBatchFile() {
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    // Decode from the old 9-column format
    auto oldMap = MantidQt::API::loadJSONFromFile(QString::fromStdString(TEN_COL_BATCH_FILE));
    decoder.decodeBatch(&mwv, 0, oldMap);

    // Check that the result matches the new format
    QList<QVariant> expectedRowValues{"0.5", "", "13463", "13464", "4", "0.01", "0.1", "0.02", "", "4", "5", ""};
    CoderCommonTester tester;
    constexpr auto rowIndex = int{0};
    tester.checkPerAngleDefaultsRowEquals(gui, expectedRowValues, rowIndex);
  }

  void test_decodeLegacyNineColBatchFile() {
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    // Decode from the old 9-column format
    auto oldMap = MantidQt::API::loadJSONFromFile(QString::fromStdString(NINE_COL_BATCH_FILE));
    decoder.decodeBatch(&mwv, 0, oldMap);

    // Check that the result matches the new format
    QList<QVariant> expectedRowValues{"0.5", "", "13463", "13464", "4", "0.01", "0.1", "0.02", "", "4", "", ""};
    CoderCommonTester tester;
    constexpr auto rowIndex = int{0};
    tester.checkPerAngleDefaultsRowEquals(gui, expectedRowValues, rowIndex);
  }

  void test_decodeInvalidEightColBatchFile() {
    QtMainWindowView mwv;
    mwv.initLayout();
    Decoder decoder;
    // Decode from the old 9-column format
    auto oldMap = MantidQt::API::loadJSONFromFile(QString::fromStdString(EIGHT_COL_BATCH_FILE));
    TS_ASSERT_THROWS(decoder.decodeBatch(&mwv, 0, oldMap), std::out_of_range const &);
  }

  void test_decodeCurrentVersionFiles() {
    auto map = MantidQt::API::loadJSONFromFile(QString::fromStdString(BATCH_FILE_V1));
    Decoder decoder;
    auto constexpr expectedVersion = 1;
    TS_ASSERT_EQUALS(expectedVersion, decoder.decodeVersion(map));
  }

  void test_decodeVersionLegacy() {
    auto map = MantidQt::API::loadJSONFromFile(QString::fromStdString(TEN_COL_BATCH_FILE));
    Decoder decoder;
    auto constexpr expectedVersion = 0;
    TS_ASSERT_EQUALS(expectedVersion, decoder.decodeVersion(map));
  }
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
