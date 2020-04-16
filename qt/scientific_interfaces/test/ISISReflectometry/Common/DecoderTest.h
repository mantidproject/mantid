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
#include "MantidPythonInterface/core/WrapPython.h"
#include "MantidQtWidgets/Common/QtJSONUtils.h"

#include <cxxtest/TestSuite.h>

#include <QMap>
#include <QString>
#include <QVariant>

namespace {
const static QString BATCH_JSON_STRING{
    "{"
    "    \"eventView\": {"
    "        \"customButton\": false,"
    "        \"customEdit\": \"\","
    "        \"disabledSlicingButton\": false,"
    "        \"logValueButton\": false,"
    "        \"logValueEdit\": \"\","
    "        \"logValueTypeEdit\": \"\","
    "        \"uniformButton\": false,"
    "        \"uniformEdit\": 1,"
    "        \"uniformEvenButton\": true,"
    "        \"uniformEvenEdit\": 2"
    "    },"
    "    \"experimentView\": {"
    "        \"analysisModeComboBox\": 1,"
    "        \"backgroundMethodComboBox\": 1,"
    "        \"costFunctionComboBox\": 1,"
    "        \"debugCheckbox\": true,"
    "        \"endOverlapEdit\": 13,"
    "        \"floodCorComboBox\": 1,"
    "        \"floodWorkspaceWsSelector\": 0,"
    "        \"includePartialBinsCheckBox\": true,"
    "        \"perAngleDefaults\": {"
    "            \"columnsNum\": 10,"
    "            \"rows\": ["
    "                ["
    "                    \"0.5\","
    "                    \"13463\","
    "                    \"13464\","
    "                    \"4\","
    "                    \"0.01\","
    "                    \"0.1\","
    "                    \"0.02\","
    "                    \"\","
    "                    \"4\","
    "                    \"5\""
    "                ]"
    "            ],"
    "            \"rowsNum\": 1"
    "        },"
    "        \"polCorrCheckBox\": false,"
    "        \"polynomialDegreeSpinBox\": 3,"
    "        \"reductionTypeComboBox\": 2,"
    "        \"startOverlapEdit\": 8,"
    "        \"stitchEdit\": \"Params=0.015\","
    "        \"subtractBackgroundCheckBox\": true,"
    "        \"summationTypeComboBox\": 1,"
    "        \"transScaleRHSCheckBox\": false,"
    "        \"transStitchParamsEdit\": \"0.03\""
    "    },"
    "    \"instrumentView\": {"
    "        \"I0MonitorIndex\": 1,"
    "        \"correctDetectorsCheckBox\": true,"
    "        \"detectorCorrectionTypeComboBox\": 1,"
    "        \"intMonCheckBox\": true,"
    "        \"lamMaxEdit\": 16,"
    "        \"lamMinEdit\": 2.5,"
    "        \"monBgMaxEdit\": 19,"
    "        \"monBgMinEdit\": 14,"
    "        \"monIntMaxEdit\": 11,"
    "        \"monIntMinEdit\": 3"
    "    },"
    "    \"runsView\": {"
    "        \"comboSearchInstrument\": 0,"
    "        \"runsTable\": {"
    "            \"filterBox\": \"\","
    "            \"projectSave\": false,"
    "            \"runsTableModel\": ["
    "                {"
    "                    \"itemState\": 0,"
    "                    \"name\": \"Si/D2O S2 \","
    "                    \"postprocessedWorkspaceName\": \"\","
    "                    \"rows\": ["
    "                        {"
    "                            \"itemState\": 0,"
    "                            \"qRange\": {"
    "                                \"max\": 0.06,"
    "                                \"maxPresent\": true,"
    "                                \"min\": 0.01,"
    "                                \"minPresent\": true,"
    "                                \"step\": 0.04,"
    "                                \"stepPresent\": true"
    "                            },"
    "                            \"qRangeOutput\": {"
    "                                \"maxPresent\": false,"
    "                                \"minPresent\": false,"
    "                                \"stepPresent\": false"
    "                            },"
    "                            \"reductionOptions\": {"
    "                            },"
    "                            \"reductionWorkspaces\": {"
    "                                \"iVsLambda\": \"\","
    "                                \"iVsQ\": \"\","
    "                                \"iVsQBinned\": \"\","
    "                                \"inputRunNumbers\": ["
    "                                    \"13460\""
    "                                ],"
    "                                \"transPair\": {"
    "                                    \"firstTransRuns\": ["
    "                                        \"13463\""
    "                                    ],"
    "                                    \"secondTransRuns\": ["
    "                                        \"13464\""
    "                                    ]"
    "                                }"
    "                            },"
    "                            \"runNumbers\": ["
    "                                \"13460\""
    "                            ],"
    "                            \"scaleFactorPresent\": false,"
    "                            \"theta\": 0.5,"
    "                            \"transRunNums\": {"
    "                                \"firstTransRuns\": ["
    "                                    \"13463\""
    "                                ],"
    "                                \"secondTransRuns\": ["
    "                                    \"13464\""
    "                                ]"
    "                            }"
    "                        },"
    "                        {"
    "                            \"itemState\": 0,"
    "                            \"qRange\": {"
    "                                \"max\": 0.3,"
    "                                \"maxPresent\": true,"
    "                                \"min\": 0.035,"
    "                                \"minPresent\": true,"
    "                                \"step\": 0.04,"
    "                                \"stepPresent\": true"
    "                            },"
    "                            \"qRangeOutput\": {"
    "                                \"maxPresent\": false,"
    "                                \"minPresent\": false,"
    "                                \"stepPresent\": false"
    "                            },"
    "                            \"reductionOptions\": {"
    "                            },"
    "                            \"reductionWorkspaces\": {"
    "                                \"iVsLambda\": \"\","
    "                                \"iVsQ\": \"\","
    "                                \"iVsQBinned\": \"\","
    "                                \"inputRunNumbers\": ["
    "                                    \"13462\""
    "                                ],"
    "                                \"transPair\": {"
    "                                    \"firstTransRuns\": ["
    "                                        \"13463\""
    "                                    ],"
    "                                    \"secondTransRuns\": ["
    "                                        \"13464\""
    "                                    ]"
    "                                }"
    "                            },"
    "                            \"runNumbers\": ["
    "                                \"13462\""
    "                            ],"
    "                            \"scaleFactorPresent\": false,"
    "                            \"theta\": 2.3,"
    "                            \"transRunNums\": {"
    "                                \"firstTransRuns\": ["
    "                                    \"13463\""
    "                                ],"
    "                                \"secondTransRuns\": ["
    "                                    \"13464\""
    "                                ]"
    "                            }"
    "                        }"
    "                    ]"
    "                },"
    "                {"
    "                    \"itemState\": 0,"
    "                    \"name\": \"Si MAB 500mg/L NaOAc D2O \","
    "                    \"postprocessedWorkspaceName\": \"\","
    "                    \"rows\": ["
    "                        {"
    "                            \"itemState\": 0,"
    "                            \"qRange\": {"
    "                                \"max\": 0.06,"
    "                                \"maxPresent\": true,"
    "                                \"min\": 0.01,"
    "                                \"minPresent\": true,"
    "                                \"step\": 0.04,"
    "                                \"stepPresent\": true"
    "                            },"
    "                            \"qRangeOutput\": {"
    "                                \"maxPresent\": false,"
    "                                \"minPresent\": false,"
    "                                \"stepPresent\": false"
    "                            },"
    "                            \"reductionOptions\": {"
    "                            },"
    "                            \"reductionWorkspaces\": {"
    "                                \"iVsLambda\": \"\","
    "                                \"iVsQ\": \"\","
    "                                \"iVsQBinned\": \"\","
    "                                \"inputRunNumbers\": ["
    "                                    \"13469\""
    "                                ],"
    "                                \"transPair\": {"
    "                                    \"firstTransRuns\": ["
    "                                        \"13463\""
    "                                    ],"
    "                                    \"secondTransRuns\": ["
    "                                        \"13464\""
    "                                    ]"
    "                                }"
    "                            },"
    "                            \"runNumbers\": ["
    "                                \"13469\""
    "                            ],"
    "                            \"scaleFactorPresent\": false,"
    "                            \"theta\": 0.7,"
    "                            \"transRunNums\": {"
    "                                \"firstTransRuns\": ["
    "                                    \"13463\""
    "                                ],"
    "                                \"secondTransRuns\": ["
    "                                    \"13464\""
    "                                ]"
    "                            }"
    "                        },"
    "                        {"
    "                            \"itemState\": 0,"
    "                            \"qRange\": {"
    "                                \"max\": 0.3,"
    "                                \"maxPresent\": true,"
    "                                \"min\": 0.035,"
    "                                \"minPresent\": true,"
    "                                \"step\": 0.04,"
    "                                \"stepPresent\": true"
    "                            },"
    "                            \"qRangeOutput\": {"
    "                                \"maxPresent\": false,"
    "                                \"minPresent\": false,"
    "                                \"stepPresent\": false"
    "                            },"
    "                            \"reductionOptions\": {"
    "                            },"
    "                            \"reductionWorkspaces\": {"
    "                                \"iVsLambda\": \"\","
    "                                \"iVsQ\": \"\","
    "                                \"iVsQBinned\": \"\","
    "                                \"inputRunNumbers\": ["
    "                                    \"13470\""
    "                                ],"
    "                                \"transPair\": {"
    "                                    \"firstTransRuns\": ["
    "                                        \"13463\""
    "                                    ],"
    "                                    \"secondTransRuns\": ["
    "                                        \"13464\""
    "                                    ]"
    "                                }"
    "                            },"
    "                            \"runNumbers\": ["
    "                                \"13470\""
    "                            ],"
    "                            \"scaleFactorPresent\": false,"
    "                            \"theta\": 2.3,"
    "                            \"transRunNums\": {"
    "                                \"firstTransRuns\": ["
    "                                    \"13463\""
    "                                ],"
    "                                \"secondTransRuns\": ["
    "                                    \"13464\""
    "                                ]"
    "                            }"
    "                        }"
    "                    ]"
    "                }"
    "            ]"
    "        },"
    "        \"textCycle\": \"11_3\","
    "        \"textSearch\": \"1120015\""
    "    },"
    "    \"saveView\": {"
    "        \"commaRadioButton\": false,"
    "        \"fileFormatComboBox\": 1,"
    "        \"filterEdit\": \"IvsQ\","
    "        \"prefixEdit\": \"\","
    "        \"qResolutionCheckBox\": true,"
    "        \"regexCheckBox\": true,"
    "        \"savePathEdit\": \"\","
    "        \"saveReductionResultsCheckBox\": false,"
    "        \"spaceRadioButton\": true,"
    "        \"tabRadioButton\": false,"
    "        \"titleCheckBox\": true"
    "    }"
    "}"};

const static QString EMPTY_BATCH_JSON_STRING{
    "{"
    "    \"eventView\": {"
    "        \"customButton\": false,"
    "        \"customEdit\": \"\","
    "        \"disabledSlicingButton\": true,"
    "        \"logValueButton\": false,"
    "        \"logValueEdit\": \"\","
    "        \"logValueTypeEdit\": \"\","
    "        \"uniformButton\": false,"
    "        \"uniformEdit\": 1,"
    "        \"uniformEvenButton\": false,"
    "        \"uniformEvenEdit\": 1"
    "    },"
    "    \"experimentView\": {"
    "        \"analysisModeComboBox\": 0,"
    "        \"backgroundMethodComboBox\": 0,"
    "        \"costFunctionComboBox\": 0,"
    "        \"debugCheckbox\": false,"
    "        \"endOverlapEdit\": 12,"
    "        \"floodCorComboBox\": 0,"
    "        \"floodWorkspaceWsSelector\": 0,"
    "        \"includePartialBinsCheckBox\": false,"
    "        \"perAngleDefaults\": {"
    "            \"columnsNum\": 10,"
    "            \"rows\": ["
    "                ["
    "                    \"\","
    "                    \"\","
    "                    \"\","
    "                    \"\","
    "                    \"\","
    "                    \"\","
    "                    \"\","
    "                    \"\","
    "                    \"\","
    "                    \"\""
    "                ]"
    "            ],"
    "            \"rowsNum\": 1"
    "        },"
    "        \"polCorrCheckBox\": false,"
    "        \"polynomialDegreeSpinBox\": 3,"
    "        \"reductionTypeComboBox\": 0,"
    "        \"startOverlapEdit\": 10,"
    "        \"stitchEdit\": \"\","
    "        \"subtractBackgroundCheckBox\": false,"
    "        \"summationTypeComboBox\": 0,"
    "        \"transScaleRHSCheckBox\": true,"
    "        \"transStitchParamsEdit\": \"\""
    "    },"
    "    \"instrumentView\": {"
    "        \"I0MonitorIndex\": 2,"
    "        \"correctDetectorsCheckBox\": true,"
    "        \"detectorCorrectionTypeComboBox\": 0,"
    "        \"intMonCheckBox\": true,"
    "        \"lamMaxEdit\": 17,"
    "        \"lamMinEdit\": 1.5,"
    "        \"monBgMaxEdit\": 18,"
    "        \"monBgMinEdit\": 17,"
    "        \"monIntMaxEdit\": 10,"
    "        \"monIntMinEdit\": 4"
    "    },"
    "    \"runsView\": {"
    "        \"comboSearchInstrument\": 0,"
    "        \"runsTable\": {"
    "            \"filterBox\": \"\","
    "            \"projectSave\": false,"
    "            \"runsTableModel\": ["
    "                {"
    "                    \"itemState\": 0,"
    "                    \"name\": \"HiddenGroupName1\","
    "                    \"postprocessedWorkspaceName\": \"\","
    "                    \"rows\": ["
    "                        {"
    "                        }"
    "                    ]"
    "                }"
    "            ]"
    "        },"
    "        \"textCycle\": \"\","
    "        \"textSearch\": \"\""
    "    },"
    "    \"saveView\": {"
    "        \"commaRadioButton\": true,"
    "        \"fileFormatComboBox\": 0,"
    "        \"filterEdit\": \"\","
    "        \"prefixEdit\": \"\","
    "        \"qResolutionCheckBox\": false,"
    "        \"regexCheckBox\": false,"
    "        \"savePathEdit\": \"\","
    "        \"saveReductionResultsCheckBox\": false,"
    "        \"spaceRadioButton\": false,"
    "        \"tabRadioButton\": false,"
    "        \"titleCheckBox\": false"
    "    }"
    "}"};

const static QString MAINWINDOW_JSON_STRING{
    QString("{\"batches\": [") + BATCH_JSON_STRING + QString(", ") +
    EMPTY_BATCH_JSON_STRING + QString("], ") +
    QString("\"tag\": \"ISIS Reflectometry\"}")};
} // namespace

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
namespace ISISReflectometry {
class DecoderTest : public CxxTest::TestSuite {
public:
  static DecoderTest *createSuite() { return new DecoderTest(); }
  static void destroySuite(DecoderTest *suite) { delete suite; }

  DecoderTest() {
    PyRun_SimpleString("import mantid.api as api\n"
                       "api.FrameworkManager.Instance()");
  }

  void test_decodeMainWindow() {
    CoderCommonTester tester;
    Decoder decoder;
    auto map = MantidQt::API::loadJSONFromString(MAINWINDOW_JSON_STRING);
    auto widget = decoder.decode(map, "");

    tester.testMainWindowView(dynamic_cast<QtMainWindowView *>(widget), map);
  }

  void test_decodeEmptyBatch() {
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONFromString(EMPTY_BATCH_JSON_STRING);
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    decoder.decodeBatch(&mwv, 0, map);

    tester.testBatch(gui, &mwv, map);
  }

  void test_decodePopulatedBatch() {
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONFromString(BATCH_JSON_STRING);
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    decoder.decodeBatch(&mwv, 0, map);

    tester.testBatch(gui, &mwv, map);
  }

  void test_decodeBatchWhenInstrumentChanged() {
    CoderCommonTester tester;
    auto map = MantidQt::API::loadJSONFromString(BATCH_JSON_STRING);
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
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
