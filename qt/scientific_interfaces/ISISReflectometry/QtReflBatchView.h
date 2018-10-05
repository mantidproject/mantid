// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_QTREFLBATCHVIEW_H
#define MANTID_ISISREFLECTOMETRY_QTREFLBATCHVIEW_H

#include "GUI/Event/EventView.h"
#include "GUI/Experiment/ExperimentView.h"
#include "GUI/Instrument/InstrumentView.h"
#include "IReflBatchPresenter.h"
#include "IReflBatchView.h"
#include "MantidAPI/IAlgorithm.h"
#include "QtReflRunsTabView.h"
#include "QtReflSaveTabView.h"
#include "ui_ReflBatchWidget.h"
#include <memory>

#include <QCloseEvent>

namespace MantidQt {
namespace CustomInterfaces {

class QtReflBatchView : public QWidget, public IReflBatchView {
  Q_OBJECT
public:
  explicit QtReflBatchView(QWidget *parent = nullptr);
  void subscribe(IReflBatchPresenter *notifyee);

  IReflRunsTabView *runs() const override;
  IEventView *eventHandling() const override;
  IReflSaveTabView *save() const override;
  IExperimentView *experiment() const override;
  IInstrumentView *instrument() const override;

private:
  void initLayout();
  Mantid::API::IAlgorithm_sptr createReductionAlg();

  std::unique_ptr<QtReflRunsTabView> createRunsTab();
  std::unique_ptr<EventView> createEventTab();
  std::unique_ptr<QtReflSaveTabView> createSaveTab();

  Ui::ReflBatchWidget m_ui;
  IReflBatchPresenter *m_notifyee;
  std::unique_ptr<QtReflRunsTabView> m_runs;
  std::unique_ptr<EventView> m_eventHandling;
  std::unique_ptr<QtReflSaveTabView> m_save;
  std::unique_ptr<ExperimentView> m_experiment;
  std::unique_ptr<InstrumentView> m_instrument;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_QTREFLBATCHVIEW_H */
