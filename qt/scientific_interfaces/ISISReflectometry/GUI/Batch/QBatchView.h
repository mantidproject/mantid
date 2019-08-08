// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_QBATCHVIEW_H
#define MANTID_ISISREFLECTOMETRY_QBATCHVIEW_H

#include "GUI/Event/QEventView.h"
#include "GUI/Experiment/QExperimentView.h"
#include "GUI/Instrument/QInstrumentView.h"
#include "GUI/Runs/QRunsView.h"
#include "GUI/Save/QSaveView.h"
#include "IBatchView.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "ui_BatchWidget.h"

#include <QCloseEvent>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class QBatchView : public QWidget, public IBatchView {
  Q_OBJECT
public:
  explicit QBatchView(QWidget *parent);
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

  std::unique_ptr<QRunsView> createRunsTab();
  std::unique_ptr<QEventView> createEventTab();
  std::unique_ptr<QSaveView> createSaveTab();

  Ui::BatchWidget m_ui;
  BatchViewSubscriber *m_notifyee;
  std::unique_ptr<QRunsView> m_runs;
  std::unique_ptr<QEventView> m_eventHandling;
  std::unique_ptr<QSaveView> m_save;
  std::unique_ptr<QExperimentView> m_experiment;
  std::unique_ptr<QInstrumentView> m_instrument;
  API::BatchAlgorithmRunner m_batchAlgoRunner;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_QBATCHVIEW_H */
