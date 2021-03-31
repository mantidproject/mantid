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
#include "GUI/Runs/RunsPresenterFactory.h"
#include "GUI/Save/SavePresenterFactory.h"
#include "IBatchPresenter.h"
#include "IBatchPresenterFactory.h"
#include "IBatchView.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class BatchPresenterFactory : public IBatchPresenterFactory {
public:
  BatchPresenterFactory(
      // cppcheck-suppress passedByValue
      RunsPresenterFactory runsPresenterFactory, EventPresenterFactory eventPresenterFactory,
      ExperimentPresenterFactory experimentPresenterFactory, InstrumentPresenterFactory instrumentPresenterFactory,
      SavePresenterFactory savePresenterFactory)
      : m_runsPresenterFactory(std::move(runsPresenterFactory)),
        m_eventPresenterFactory(std::move(eventPresenterFactory)),
        m_experimentPresenterFactory(std::move(experimentPresenterFactory)),
        m_instrumentPresenterFactory(std::move(instrumentPresenterFactory)),
        m_savePresenterFactory(std::move(savePresenterFactory)) {}

  std::unique_ptr<IBatchPresenter> make(IBatchView *view) override {
    auto runsPresenter = m_runsPresenterFactory.make(view->runs());
    auto eventPresenter = m_eventPresenterFactory.make(view->eventHandling());
    auto experimentPresenter = m_experimentPresenterFactory.make(view->experiment());
    auto instrumentPresenter = m_instrumentPresenterFactory.make(view->instrument());
    auto savePresenter = m_savePresenterFactory.make(view->save());

    auto model = Batch(experimentPresenter->experiment(), instrumentPresenter->instrument(),
                       runsPresenter->mutableRunsTable(), eventPresenter->slicing());

    return std::make_unique<BatchPresenter>(view, std::move(model), std::move(runsPresenter), std::move(eventPresenter),
                                            std::move(experimentPresenter), std::move(instrumentPresenter),
                                            std::move(savePresenter));
  }

private:
  RunsPresenterFactory m_runsPresenterFactory;
  EventPresenterFactory m_eventPresenterFactory;
  ExperimentPresenterFactory m_experimentPresenterFactory;
  InstrumentPresenterFactory m_instrumentPresenterFactory;
  SavePresenterFactory m_savePresenterFactory;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt