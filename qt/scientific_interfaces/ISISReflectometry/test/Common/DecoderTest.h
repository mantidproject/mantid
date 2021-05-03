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
const static QString BATCH_JSON_STRING{"{"
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
                                       "        \"searchResults\": ["
                                       "           {"
                                       "               \"comment\": \"Annotated valid run\","
                                       "               \"error\": \"\","
                                       "               \"excludeReason\": \"\","
                                       "               \"groupName\": \"Test run th=0.5\","
                                       "               \"runNumber\": \"13460\","
                                       "               \"theta\": \"0.5\","
                                       "               \"title\": \"Test run th=0.5\""
                                       "           },"
                                       "           {"
                                       "               \"comment\": \"\","
                                       "               \"error\": \"\","
                                       "               \"excludeReason\": \"User excluded run\","
                                       "               \"groupName\": \"Test run 2 th=0.7\","
                                       "               \"runNumber\": \"13462\","
                                       "               \"theta\": \"0.5\","
                                       "               \"title\": \"Test run 2 th=0.7\""
                                       "           },"
                                       "           {"
                                       "               \"comment\": \"\","
                                       "               \"error\": \"Theta was not specified in the run title.\","
                                       "               \"excludeReason\": \"\","
                                       "               \"groupName\": \"Direct Beam\","
                                       "               \"runNumber\": \"13463\","
                                       "               \"theta\": \"\","
                                       "               \"title\": \"Direct Beam\""
                                       "           }"
                                       "        ],"
                                       "        \"textCycle\": \"11_3\","
                                       "        \"textSearch\": \"1120015\","
                                       "        \"textInstrument\": \"INTER\""
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
                                       "        \"headerCheckBox\": true"
                                       "    }"
                                       "}"};

const static QString EMPTY_EVENT_JSON_STRING{"{"
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
                                             "    },"};

const static QString EMPTY_EXPERIMENT_JSON_STRING{"    \"experimentView\": {"
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
                                                  "    },"};

const static QString EMPTY_INSTRUMENT_JSON_STRING{"    \"instrumentView\": {"
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
                                                  "    },"};

const static QString EMPTY_RUNS_JSON_STRING{"    \"runsView\": {"
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
                                            "        \"searchResults\": [],"
                                            "        \"textCycle\": \"\","
                                            "        \"textSearch\": \"\","
                                            "        \"textInstrument\": \"\""
                                            "    },"};

const static QString EMPTY_SAVE_JSON_STRING{"    \"saveView\": {"
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
                                            "        \"headerCheckBox\": false"
                                            "    }"
                                            "}"};

const static QString EMPTY_BATCH_JSON_STRING{EMPTY_EVENT_JSON_STRING + EMPTY_EXPERIMENT_JSON_STRING +
                                             EMPTY_INSTRUMENT_JSON_STRING + EMPTY_RUNS_JSON_STRING +
                                             EMPTY_SAVE_JSON_STRING};

// This batch file has an incorrect number of columns (9 instead of 10) for the
// experiment tab's table - this needs to be supported for backwards
// compatibility
const static QString EXPERIMENT_JSON_STRING_9_COLUMNS{"    \"experimentView\": {"
                                                      "        \"analysisModeComboBox\": 0,"
                                                      "        \"backgroundMethodComboBox\": 0,"
                                                      "        \"costFunctionComboBox\": 0,"
                                                      "        \"debugCheckbox\": false,"
                                                      "        \"endOverlapEdit\": 12,"
                                                      "        \"floodCorComboBox\": 0,"
                                                      "        \"floodWorkspaceWsSelector\": 0,"
                                                      "        \"includePartialBinsCheckBox\": false,"
                                                      "        \"perAngleDefaults\": {"
                                                      "            \"columnsNum\": 9,"
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
                                                      "    },"};

const static QString BATCH_JSON_STRING_9_COLUMNS{EMPTY_EVENT_JSON_STRING + EXPERIMENT_JSON_STRING_9_COLUMNS +
                                                 EMPTY_INSTRUMENT_JSON_STRING + EMPTY_RUNS_JSON_STRING +
                                                 EMPTY_SAVE_JSON_STRING};

const static QString MAINWINDOW_JSON_STRING{QString("{\"batches\": [") + BATCH_JSON_STRING + QString(", ") +
                                            EMPTY_BATCH_JSON_STRING + QString("], ") +
                                            QString("\"tag\": \"ISIS Reflectometry\"}")};
} // namespace

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

  void test_decodeOldBatchFile() {
    CoderCommonTester tester;
    QtMainWindowView mwv;
    mwv.initLayout();
    auto gui = dynamic_cast<QtBatchView *>(mwv.batches()[0]);
    Decoder decoder;
    // Decode from the old 9-column format
    auto oldMap = MantidQt::API::loadJSONFromString(BATCH_JSON_STRING_9_COLUMNS);
    decoder.decodeBatch(&mwv, 0, oldMap);
    // Check that the result matches the new 10-column format
    auto newMap = MantidQt::API::loadJSONFromString(EMPTY_BATCH_JSON_STRING);
    tester.testBatch(gui, &mwv, newMap);
  }
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
