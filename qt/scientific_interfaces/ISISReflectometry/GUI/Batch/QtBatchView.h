// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_QTBATCHVIEW_H
#define MANTID_ISISREFLECTOMETRY_QTBATCHVIEW_H

#include "GUI/Event/QtEventView.h"
#include "GUI/Experiment/QtExperimentView.h"
#include "GUI/Instrument/QtInstrumentView.h"
#include "GUI/Runs/QtRunsView.h"
#include "GUI/Save/QtSaveView.h"
#include "IBatchView.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "ui_BatchWidget.h"

#include <QCloseEvent>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL QtBatchView : public QWidget,
                                                   public IBatchView {
  Q_OBJECT
public:
  explicit QtBatchView(QWidget *parent);
  void subscribe(BatchViewSubscriber *notifyee) override;

  IRunsView *runs() const override;
  IEventView *eventHandling() const override;
  ISaveView *save() const override;
  IExperimentView *experiment() const override;
  IInstrumentView *instrument() const override;
  void clearAlgorithmQueue() override;
  void setAlgorithmQueue(
      std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithms) override;
  void executeAlgorithmQueue() override;
  void cancelAlgorithmQueue() override;

private slots:
  void onBatchComplete(bool error);
  void onBatchCancelled();
  void onAlgorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr algorithm);
  void onAlgorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr algorithm);
  void onAlgorithmError(MantidQt::API::IConfiguredAlgorithm_sptr algorithm,
                        std::string errorMessage);

private:
  void initLayout();
  Mantid::API::IAlgorithm_sptr createReductionAlg();
  void connectBatchAlgoRunnerSlots();

  std::unique_ptr<QtRunsView> createRunsTab();
  std::unique_ptr<QtEventView> createEventTab();
  std::unique_ptr<QtSaveView> createSaveTab();

  Ui::BatchWidget m_ui;
  BatchViewSubscriber *m_notifyee;
  std::unique_ptr<QtRunsView> m_runs;
  std::unique_ptr<QtEventView> m_eventHandling;
  std::unique_ptr<QtSaveView> m_save;
  std::unique_ptr<QtExperimentView> m_experiment;
  std::unique_ptr<QtInstrumentView> m_instrument;
  API::BatchAlgorithmRunner m_batchAlgoRunner;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_QTBATCHVIEW_H */
