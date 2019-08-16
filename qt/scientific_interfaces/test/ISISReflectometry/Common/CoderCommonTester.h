// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef CODER_COMMON_TESTER_H_
#define CODER_COMMON_TESTER_H_

#include "../../../ISISReflectometry/GUI/Batch/QtBatchView.h"
#include "../../../ISISReflectometry/GUI/Common/Decoder.h"
#include "../../../ISISReflectometry/GUI/Experiment/QtExperimentView.h"
#include "../../../ISISReflectometry/GUI/Instrument/QtInstrumentView.h"
#include "../../../ISISReflectometry/GUI/MainWindow/QtMainWindowView.h"
#include "../../../ISISReflectometry/GUI/Runs/QtRunsView.h"
#include "../../../ISISReflectometry/GUI/RunsTable/QtRunsTableView.h"
#include "../../../ISISReflectometry/GUI/RunsTable/RunsTablePresenter.h"
#include "../../../ISISReflectometry/GUI/Save/QtSaveView.h"
#include "../../../ISISReflectometry/Reduction/Group.h"
#include "../../../ISISReflectometry/Reduction/ReductionJobs.h"
#include "../../../ISISReflectometry/Reduction/ReductionWorkspaces.h"
#include "../../../ISISReflectometry/Reduction/Row.h"

#include <QMap>
#include <QString>
#include <QTableWidget>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class CoderCommonTester {
public:
  void testMainWindowView(const QtMainWindowView &mwv,
                          const QMap<QString, QVariant> &map) {
    auto list = map[QString("batches")].toList();
    for (auto batchIndex = 0u; batchIndex < mwv.m_batchViews.size();
         ++batchIndex) {
      testBatch(mwv.m_batchViews[batchIndex], mwv, list[batchIndex].toMap());
    }
    TS_ASSERT_EQUALS(map[QString("tag")].toString().toStdString(),
                     "ISIS Reflectometry")
  }

  void testBatch(const QtBatchView *gui, const QtMainWindowView &mwv,
                 const QMap<QString, QVariant> &map) {
    Decoder batchFinder;
    auto batchPresenter = batchFinder.findBatchPresenter(gui, mwv);
    auto runsPresenter =
        dynamic_cast<RunsPresenter *>(batchPresenter->m_runsPresenter.get());
    auto runsTablePresenter = dynamic_cast<RunsTablePresenter *>(
        runsPresenter->m_tablePresenter.get());
    auto reductionJobs = &runsTablePresenter->m_model.m_reductionJobs;
    testRuns(gui->m_runs.get(), reductionJobs,
             map[QString("runsView")].toMap());
    testEvent(gui->m_eventHandling.get(), map[QString("eventView")].toMap());
    testExperiment(gui->m_experiment.get(),
                   map[QString("experimentView")].toMap());
    testInstrument(gui->m_instrument.get(),
                   map[QString("instrumentView")].toMap());
    testSave(gui->m_save.get(), map[QString("saveView")].toMap());
  }

private:
  void testExperiment(const QtExperimentView *gui,
                      const QMap<QString, QVariant> &map) {
    TS_ASSERT_EQUALS(gui->m_ui.analysisModeComboBox->currentIndex(),
                     map[QString("analysisModeComboBox")].toInt())
    TS_ASSERT_EQUALS(gui->m_ui.debugCheckBox->isChecked(),
                     map[QString("debugCheckbox")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.summationTypeComboBox->currentIndex(),
                     map[QString("summationTypeComboBox")].toInt())
    TS_ASSERT_EQUALS(gui->m_ui.reductionTypeComboBox->currentIndex(),
                     map[QString("reductionTypeComboBox")].toInt())
    TS_ASSERT_EQUALS(gui->m_ui.includePartialBinsCheckBox->isChecked(),
                     map[QString("includePartialBinsCheckBox")].toBool())
    testPerAngleDefaults(gui->m_ui.optionsTable,
                         map[QString("perAngleDefaults")].toMap());
    TS_ASSERT_EQUALS(gui->m_ui.startOverlapEdit->value(),
                     map[QString("startOverlapEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.endOverlapEdit->value(),
                     map[QString("endOverlapEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.transStitchParamsEdit->text(),
                     map[QString("transStitchParamsEdit")].toString())
    TS_ASSERT_EQUALS(gui->m_ui.transScaleRHSCheckBox->isChecked(),
                     map[QString("transScaleRHSCheckBox")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.polCorrCheckBox->isChecked(),
                     map[QString("polCorrCheckBox")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.floodCorComboBox->currentIndex(),
                     map[QString("floodCorComboBox")].toInt())
    TS_ASSERT_EQUALS(gui->m_ui.floodWorkspaceWsSelector->currentIndex(),
                     map[QString("floodWorkspaceWsSelector")].toInt())
    TS_ASSERT_EQUALS(gui->m_stitchEdit->text(),
                     map[QString("stitchEdit")].toString())
  }

  void testPerAngleDefaults(const QTableWidget *tab,
                            const QMap<QString, QVariant> &map) {
    TS_ASSERT_EQUALS(tab->rowCount(), map[QString("rowsNum")].toInt())
    TS_ASSERT_EQUALS(tab->columnCount(), map[QString("columnsNum")].toInt())
    testPerAngleDefaultsRows(tab, map[QString("rows")].toList());
  }

  void testPerAngleDefaultsRows(const QTableWidget *tab,
                                const QList<QVariant> &list) {
    for (auto rowIndex = 0; rowIndex < tab->rowCount(); ++rowIndex) {
      testPerAngleDefaultsRow(tab, list[rowIndex].toList(), rowIndex);
    }
  }

  void testPerAngleDefaultsRow(const QTableWidget *tab,
                               const QList<QVariant> &list, int rowIndex) {
    for (auto columnIndex = 0; columnIndex < tab->columnCount();
         ++columnIndex) {
      auto guiText = tab->item(rowIndex, columnIndex)->text();
      TS_ASSERT_EQUALS(guiText, list[columnIndex].toString())
    }
  }

  void testInstrument(const QtInstrumentView *gui,
                      const QMap<QString, QVariant> &map) {
    TS_ASSERT_EQUALS(gui->m_ui.intMonCheckBox->isChecked(),
                     map[QString("intMonCheckBox")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.monIntMinEdit->value(),
                     map[QString("monIntMinEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.monIntMaxEdit->value(),
                     map[QString("monIntMaxEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.monBgMinEdit->value(),
                     map[QString("monBgMinEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.monBgMaxEdit->value(),
                     map[QString("monBgMaxEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.lamMinEdit->value(),
                     map[QString("lamMinEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.lamMaxEdit->value(),
                     map[QString("lamMaxEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.I0MonitorIndex->value(),
                     map[QString("I0MonitorIndex")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.correctDetectorsCheckBox->isChecked(),
                     map[QString("correctDetectorsCheckBox")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.detectorCorrectionTypeComboBox->currentIndex(),
                     map[QString("detectorCorrectionTypeComboBox")].toInt())
  }

  void testRuns(const QtRunsView *gui, const ReductionJobs *redJobs,
                const QMap<QString, QVariant> &map) {
    testRunsTable(gui->m_tableView, redJobs, map[QString("runsTable")].toMap());
    TS_ASSERT_EQUALS(gui->m_ui.comboSearchInstrument->currentIndex(),
                     map[QString("comboSearchInstrument")].toInt())
    TS_ASSERT_EQUALS(gui->m_ui.textSearch->text(),
                     map[QString("textSearch")].toString())
  }

  void testRunsTable(const QtRunsTableView *gui, const ReductionJobs *redJobs,
                     const QMap<QString, QVariant> &map) {
    TS_ASSERT_EQUALS(gui->m_ui.filterBox->text(),
                     map[QString("filterBox")].toString())
    testRunsTableModel(redJobs, map[QString("runsTableModel")].toList());
  }

  void testRunsTableModel(const ReductionJobs *redJobs,
                          const QList<QVariant> &list) {
    for (auto index = 0u; index < redJobs->groups().size(); ++index) {
      testGroup(redJobs->groups()[index], list[index].toMap());
    }
  }

  void
  testGroup(const MantidQt::CustomInterfaces::ISISReflectometry::Group &group,
            const QMap<QString, QVariant> &map) {
    TS_ASSERT_EQUALS(group.name(),
                     map[QString("name")].toString().toStdString())
    TS_ASSERT_EQUALS(
        group.postprocessedWorkspaceName(),
        map[QString("postProcessedWorkspaceName")].toString().toStdString())
    testRows(group, map[QString("rows")].toList());
  }

  void
  testRows(const MantidQt::CustomInterfaces::ISISReflectometry::Group &group,
           const QList<QVariant> &list) {
    auto rows = group.rows();
    for (auto index = 0u; index < rows.size(); ++index) {
      testRow(rows[index], list[index].toMap());
    }
  }

  void testRow(
      const boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>
          &row,
      const QMap<QString, QVariant> &map) {
    if (row) {
      auto runNumbers = row.get().runNumbers();
      auto runNumbersVariants = map[QString("runNumbers")].toList();
      for (auto index = 0u; index < runNumbers.size(); ++index) {
        TS_ASSERT_EQUALS(runNumbers[index],
                         runNumbersVariants[index].toString().toStdString());
      }
      TS_ASSERT_EQUALS(row.get().theta(), map[QString("theta")].toDouble())
      testRangeInQ(row.get().qRange(), map[QString("qRange")].toMap());
      auto scaleFactorPresent = static_cast<bool>(row.get().scaleFactor());
      TS_ASSERT_EQUALS(scaleFactorPresent,
                       map[QString("scaleFactorPresent")].toBool());
      if (scaleFactorPresent) {
        TS_ASSERT_EQUALS(row.get().scaleFactor().get(),
                         map[QString("scaleFactor")].toDouble())
      }
      testTransmissionRunPair(row.get().transmissionWorkspaceNames(),
                              map[QString("transRunNums")].toMap());
      testReductionWorkspaces(row.get().reducedWorkspaceNames(),
                              map[QString("reductionWorkspaces")].toMap());
      testReductionOptions(row.get().reductionOptions(),
                           map[QString("reductionOptions")].toMap());
    } else {
      // Row is an empty boost optional so map size should be 0
      TS_ASSERT_EQUALS(0, map.size())
    }
  }

  void testRangeInQ(const RangeInQ &range, const QMap<QString, QVariant> &map) {
    auto min = range.min();
    auto max = range.max();
    auto step = range.step();
    TS_ASSERT_EQUALS(static_cast<bool>(min),
                     map[QString("minPresent")].toBool())
    TS_ASSERT_EQUALS(static_cast<bool>(max),
                     map[QString("maxPresent")].toBool())
    TS_ASSERT_EQUALS(static_cast<bool>(step),
                     map[QString("stepPresent")].toBool())
    if (min)
      TS_ASSERT_EQUALS(min.get(), map[QString("min")].toDouble())
    if (max)
      TS_ASSERT_EQUALS(max.get(), map[QString("max")].toDouble())
    if (step)
      TS_ASSERT_EQUALS(step.get(), map[QString("step")].toDouble())
  }

  void testTransmissionRunPair(const TransmissionRunPair &pair,
                               const QMap<QString, QVariant> &map) {
    std::vector<std::string> firstTransRunNums;
    for (const auto &elem : map[QString("firstTransRuns")].toList()) {
      firstTransRunNums.emplace_back(elem.toString().toStdString());
    }
    std::vector<std::string> secondTransRunNums;
    for (const auto &elem : map[QString("secondTransRuns")].toList()) {
      secondTransRunNums.emplace_back(elem.toString().toStdString());
    }
    TS_ASSERT_EQUALS(pair.firstTransmissionRunNumbers(), firstTransRunNums)
    TS_ASSERT_EQUALS(pair.secondTransmissionRunNumbers(), secondTransRunNums)
  }

  void testReductionWorkspaces(const ReductionWorkspaces &redWs,
                               const QMap<QString, QVariant> &map) {
    std::vector<std::string> inputRunNumbers;
    for (const auto &elem : map[QString("inputRunNumbers")].toList()) {
      inputRunNumbers.emplace_back(elem.toString().toStdString());
    }
    TS_ASSERT_EQUALS(redWs.inputRunNumbers(), inputRunNumbers)
    testTransmissionRunPair(redWs.transmissionRuns(),
                            map[QString("transPair")].toMap());
    TS_ASSERT_EQUALS(redWs.iVsLambda(),
                     map[QString("iVsLambda")].toString().toStdString())
    TS_ASSERT_EQUALS(redWs.iVsQ(),
                     map[QString("iVsQ")].toString().toStdString())
    TS_ASSERT_EQUALS(redWs.iVsQBinned(),
                     map[QString("iVsQBinned")].toString().toStdString())
  }

  void testReductionOptions(const ReductionOptionsMap &rom,
                            const QMap<QString, QVariant> &map) {
    QMap<QString, QVariant> rom2;
    for (const auto &elem : rom) {
      rom2.insert(QString::fromStdString(elem.first),
                  QVariant(QString::fromStdString(elem.second)));
    }
    TS_ASSERT_EQUALS(rom2, map)
  }

  void testSave(const QtSaveView *gui, const QMap<QString, QVariant> &map) {
    TS_ASSERT_EQUALS(gui->m_ui.savePathEdit->text(),
                     map[QString("savePathEdit")].toString())
    TS_ASSERT_EQUALS(gui->m_ui.prefixEdit->text(),
                     map[QString("prefixEdit")].toString())
    TS_ASSERT_EQUALS(gui->m_ui.titleCheckBox->isChecked(),
                     map[QString("titleCheckBox")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.qResolutionCheckBox->isChecked(),
                     map[QString("qResolutionCheckBox")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.commaRadioButton->isChecked(),
                     map[QString("commaRadioButton")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.spaceRadioButton->isChecked(),
                     map[QString("spaceRadioButton")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.tabRadioButton->isChecked(),
                     map[QString("tabRadioButton")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.fileFormatComboBox->currentIndex(),
                     map[QString("fileFormatComboBox")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.filterEdit->text(),
                     map[QString("filterEdit")].toString())
    TS_ASSERT_EQUALS(gui->m_ui.regexCheckBox->isChecked(),
                     map[QString("regexCheckBox")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.saveReductionResultsCheckBox->isChecked(),
                     map[QString("saveReductionResultsCheckBox")].toBool())
  }

  void testEvent(const QtEventView *gui, const QMap<QString, QVariant> &map) {
    TS_ASSERT_EQUALS(gui->m_ui.disabledSlicingButton->isChecked(),
                     map[QString("disabledSlicingButton")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.uniformEvenButton->isChecked(),
                     map[QString("uniformEvenButton")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.uniformEvenEdit->value(),
                     map[QString("uniformEvenEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.uniformButton->isChecked(),
                     map[QString("uniformButton")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.uniformEdit->value(),
                     map[QString("uniformEdit")].toDouble())
    TS_ASSERT_EQUALS(gui->m_ui.customButton->isChecked(),
                     map[QString("customButton")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.customEdit->text(),
                     map[QString("customEdit")].toString())
    TS_ASSERT_EQUALS(gui->m_ui.logValueButton->isChecked(),
                     map[QString("logValueButton")].toBool())
    TS_ASSERT_EQUALS(gui->m_ui.logValueEdit->text(),
                     map[QString("logValueEdit")].toString())
    TS_ASSERT_EQUALS(gui->m_ui.logValueTypeEdit->text(),
                     map[QString("logValueTypeEdit")].toString())
  }
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* CODER_COMMON_TESTER_H_ */