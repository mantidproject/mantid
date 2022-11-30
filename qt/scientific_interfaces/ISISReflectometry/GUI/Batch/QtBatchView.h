// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Common/IJobRunner.h"
#include "GUI/Event/QtEventView.h"
#include "GUI/Experiment/QtExperimentView.h"
#include "GUI/Instrument/QtInstrumentView.h"
#include "GUI/Preview/QtPreviewView.h"
#include "GUI/Runs/QtRunsView.h"
#include "GUI/Save/QtSaveView.h"
#include "IBatchView.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "ui_BatchWidget.h"

#include <QCloseEvent>
#include <memory>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL QtBatchView : public QWidget, public IBatchView {
  Q_OBJECT
public:
  explicit QtBatchView(QWidget *parent);

  // IBatchView overrides
  IRunsView *runs() const override;
  IEventView *eventHandling() const override;
  ISaveView *save() const override;
  IExperimentView *experiment() const override;
  IInstrumentView *instrument() const override;
  IPreviewView *preview() const override;

private:
  void initLayout();
  Mantid::API::IAlgorithm_sptr createReductionAlg();

  std::unique_ptr<QtRunsView> createRunsTab();
  std::unique_ptr<QtEventView> createEventTab();
  std::unique_ptr<QtSaveView> createSaveTab();

  Ui::BatchWidget m_ui;
  std::unique_ptr<QtRunsView> m_runs;
  std::unique_ptr<QtEventView> m_eventHandling;
  std::unique_ptr<QtSaveView> m_save;
  std::unique_ptr<QtExperimentView> m_experiment;
  std::unique_ptr<QtInstrumentView> m_instrument;
  std::unique_ptr<QtPreviewView> m_preview;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
