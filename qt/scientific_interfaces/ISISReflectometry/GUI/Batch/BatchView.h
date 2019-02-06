// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_BATCHVIEW_H
#define MANTID_ISISREFLECTOMETRY_BATCHVIEW_H

#include "GUI/Event/EventView.h"
#include "GUI/Experiment/ExperimentView.h"
#include "GUI/Instrument/InstrumentView.h"
#include "GUI/Runs/RunsView.h"
#include "GUI/Save/SaveView.h"
#include "IBatchView.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "ui_BatchWidget.h"
#include <memory>

#include <QCloseEvent>

namespace MantidQt {
namespace CustomInterfaces {

class BatchView : public QWidget, public IBatchView {
  Q_OBJECT
public:
  explicit BatchView(QWidget *parent = nullptr);
  void subscribe(BatchViewSubscriber *notifyee) override;

  IRunsView *runs() const override;
  IEventView *eventHandling() const override;
  ISaveView *save() const override;
  IExperimentView *experiment() const override;
  IInstrumentView *instrument() const override;
  API::BatchAlgorithmRunner &batchAlgorithmRunner() override;
  void executeBatchAlgorithmRunner() override;

private slots:
  void onBatchComplete(bool error);
  void onBatchCancelled();
  void
  onAlgorithmComplete(Mantid::API::IAlgorithm_sptr algorithm,
                      MantidQt::API::BatchAlgorithmRunnerSubscriber *notifyee);
  void
  onAlgorithmError(std::string const &errorMessage,
                   Mantid::API::IAlgorithm_sptr algorithm,
                   MantidQt::API::BatchAlgorithmRunnerSubscriber *notifyee);

private:
  void initLayout();
  Mantid::API::IAlgorithm_sptr createReductionAlg();

  std::unique_ptr<RunsView> createRunsTab();
  std::unique_ptr<EventView> createEventTab();
  std::unique_ptr<SaveView> createSaveTab();

  Ui::BatchWidget m_ui;
  BatchViewSubscriber *m_notifyee;
  std::unique_ptr<RunsView> m_runs;
  std::unique_ptr<EventView> m_eventHandling;
  std::unique_ptr<SaveView> m_save;
  std::unique_ptr<ExperimentView> m_experiment;
  std::unique_ptr<InstrumentView> m_instrument;
  API::BatchAlgorithmRunner m_batchAlgoRunner;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_BATCHVIEW_H */
