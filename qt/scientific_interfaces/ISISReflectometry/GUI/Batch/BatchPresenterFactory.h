// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "BatchPresenter.h"
#include "Common/DllConfig.h"
#include "GUI/Event/EventPresenterFactory.h"
#include "GUI/Experiment/ExperimentPresenterFactory.h"
#include "GUI/Instrument/InstrumentPresenterFactory.h"
#include "GUI/Preview/PreviewPresenterFactory.h"
#include "GUI/Runs/RunsPresenterFactory.h"
#include "GUI/Save/SavePresenterFactory.h"
#include "IBatchPresenter.h"
#include "IBatchPresenterFactory.h"
#include "IBatchView.h"
#include "ReflAlgorithmFactory.h"
#include <memory>

namespace MantidQt::MantidWidgets {
class IMessageHandler;
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class BatchPresenterFactory : public IBatchPresenterFactory {
public:
  BatchPresenterFactory(

      RunsPresenterFactory runsPresenterFactory, EventPresenterFactory eventPresenterFactory,
      ExperimentPresenterFactory experimentPresenterFactory, InstrumentPresenterFactory instrumentPresenterFactory,
      PreviewPresenterFactory previewPresenterFactory, SavePresenterFactory savePresenterFactory,
      MantidQt::MantidWidgets::IMessageHandler *messageHandler)
      : m_runsPresenterFactory(std::move(runsPresenterFactory)),
        m_eventPresenterFactory(std::move(eventPresenterFactory)),
        m_experimentPresenterFactory(std::move(experimentPresenterFactory)),
        m_instrumentPresenterFactory(std::move(instrumentPresenterFactory)),
        m_previewPresenterFactory(std::move(previewPresenterFactory)),
        m_savePresenterFactory(std::move(savePresenterFactory)), m_messageHandler(messageHandler) {}

  std::unique_ptr<IBatchPresenter> make(IBatchView *view) override {
    auto runsPresenter = m_runsPresenterFactory.make(view->runs());
    auto experimentPresenter = m_experimentPresenterFactory.make(view->experiment());
    auto instrumentPresenter = m_instrumentPresenterFactory.make(view->instrument());
    auto eventPresenter = m_eventPresenterFactory.make(view->eventHandling());
    auto savePresenter = m_savePresenterFactory.make(view->save());

    auto batchModel = std::make_unique<Batch>(experimentPresenter->experiment(), instrumentPresenter->instrument(),
                                              runsPresenter->mutableRunsTable(), eventPresenter->slicing());
    auto algFactory = std::make_unique<ReflAlgorithmFactory>(*batchModel);
    auto previewPresenter = m_previewPresenterFactory.make(view->preview(), view, std::move(algFactory));

    return std::make_unique<BatchPresenter>(view, std::move(batchModel), view, std::move(runsPresenter),
                                            std::move(eventPresenter), std::move(experimentPresenter),
                                            std::move(instrumentPresenter), std::move(savePresenter),
                                            std::move(previewPresenter), m_messageHandler);
  }

private:
  RunsPresenterFactory m_runsPresenterFactory;
  EventPresenterFactory m_eventPresenterFactory;
  ExperimentPresenterFactory m_experimentPresenterFactory;
  InstrumentPresenterFactory m_instrumentPresenterFactory;
  PreviewPresenterFactory m_previewPresenterFactory;
  SavePresenterFactory m_savePresenterFactory;
  MantidQt::MantidWidgets::IMessageHandler *m_messageHandler;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
