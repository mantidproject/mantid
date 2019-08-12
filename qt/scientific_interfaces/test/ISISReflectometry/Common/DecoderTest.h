// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef ISISREFLECTOMETRY_TEST_DECODER_TEST_H_
#define ISISREFLECTOMETRY_TEST_DECODER_TEST_H_

#include "../../../ISISReflectometry/GUI/Common/Decoder.h"
#include "../ReflMockObjects.h"
#include "CoderCommonTester.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/QtJSONUtils.h"

#include <cxxtest/TestSuite.h>

#include <QMap>
#include <QString>
#include <QVariant>

namespace {
const static QString JSON_STRING{
    "{\"eventView\":{\"customButton\":false,\"customEdit\":\"\", "
    "\"disabledSlicingButton\":false,\"logValueButton\":false,\"logValueEdit\":"
    "\"\",\"logValueTypeEdit\":\"\",\"uniformButton\":true,\"uniformEdit\":3,"
    "\"uniformEvenButton\":false,\"uniformEvenEdit\":2},\"experimentView\":{"
    "\"analysisModeComboBox\":1,\"debugCheckbox\":true,\"endOverlapEdit\":13,"
    "\"floodCorComboBox\":1,\"floodWorkspaceWsSelector\":0,"
    "\"includePartialBinsCheckBox\":true,\"perAngleDefaults\":{\"columnsNum\":"
    "9,\"rows\":[[\"123\",\"123\",\"213\",\"123\",\"123\",\"123\",\"123\","
    "\"123\",\"123\"]],\"rowsNum\":1},\"polCorrCheckBox\":false,"
    "\"reductionTypeComboBox\":2,\"startOverlapEdit\":12,\"stitchEdit\":\"\","
    "\"summationTypeComboBox\":1,\"transScaleRHSCheckBox\":true,"
    "\"transStitchParamsEdit\":\"1\"},\"instrumentView\":{\"I0MonitorIndex\":3,"
    "\"correctDetectorsCheckBox\":false,\"detectorCorrectionTypeComboBox\":1,"
    "\"intMonCheckBox\":false,\"lamMaxEdit\":18,\"lamMinEdit\":2.5,"
    "\"monBgMaxEdit\":20,\"monBgMinEdit\":19,\"monIntMaxEdit\":11,"
    "\"monIntMinEdit\":5},\"runsView\":{\"comboSearchInstrument\":0,"
    "\"runsTable\":{\"filterBox\":\"123\",\"projectSave\":false,"
    "\"runsTableModel\":[{\"name\":\"123\",\"postprocessedWorkspaceName\":\"\","
    "\"rows\":[{\"qRange\":{\"max\":0.1,\"maxPresent\":true,\"min\":0.1,"
    "\"minPresent\":true,\"step\":0.1,\"stepPresent\":true},\"qRangeOutput\":{"
    "\"maxPresent\":false,\"minPresent\":false,\"stepPresent\":false},"
    "\"reductionOptions\":{\"ProcessingInstructions\":\"1-4\"},"
    "\"reductionWorkspaces\":{\"iVsLambda\":\"\",\"iVsQ\":\"\",\"iVsQBinned\":"
    "\"\",\"inputRunNumbers\":[\"13460\"],\"transPair\":{\"firstTransRuns\":["
    "\"123\"],\"secondTransRuns\":[\"1234\"]}},\"runNumbers\":[\"13460\"],"
    "\"scaleFactor\":2,\"scaleFactorPresent\":true,\"theta\":0.5,"
    "\"transRunNums\":{\"firstTransRuns\":[\"123\"],\"secondTransRuns\":["
    "\"1234\"]}}]}]},\"textSearch\":\"1120015\"},\"saveView\":{"
    "\"commaRadioButton\":false,\"fileFormatComboBox\":1,\"filterEdit\":"
    "\"123\",\"prefixEdit\":\"123\",\"qResolutionCheckBox\":true,"
    "\"regexCheckBox\":true,\"savePathEdit\":\"123\","
    "\"saveReductionResultsCheckBox\":false,\"spaceRadioButton\":true,"
    "\"tabRadioButton\":false,\"titleCheckBox\":true}}"};
}

/**
 * QApplication
 *
 * Uses setUpWorld/tearDownWorld to initialize & finalize
 * QApplication object
 */
class QApplicationHolder : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    m_app = new QApplication(m_argc, m_argv);

    qRegisterMetaType<std::string>("StdString");
    qRegisterMetaType<Mantid::API::Workspace_sptr>("Workspace");

    return true;
  }

  bool tearDownWorld() override {
    delete m_app;
    return true;
  }

  int m_argc = 1;
  GNU_DIAG_OFF("pedantic")
  char *m_argv[1] = {"DecoderTest"};
  GNU_DIAG_ON("pedantic")
  QApplication *m_app;
};

static QApplicationHolder MAIN_QAPPLICATION;

namespace MantidQt {
namespace CustomInterfaces {
class DecoderTest : public CxxTest::TestSuite {
public:
  static DecoderTest *createSuite() { return new DecoderTest(); }
  static void destroySuite(DecoderTest *suite) { delete suite; }

  void setUp() override { Mantid::API::FrameworkManager::Instance(); }

  void test_decodeBatch() {
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONfromString(JSON_STRING);
    MantidQt::CustomInterfaces::MainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<BatchView *>(mwv.batches()[0]);
    MantidQt::CustomInterfaces::Decoder decoder;
    decoder.decodeBatch(gui, mwv, map);

    tester.testBatch(gui, mwv, map);
  }
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* ISISREFLECTOMETRY_TEST_DECODER_TEST_H_ */