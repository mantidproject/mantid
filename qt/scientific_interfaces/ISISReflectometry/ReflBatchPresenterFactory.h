#ifndef MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTERFACTORY_H
#include "DllConfig.h"
#include "GUI/Event/EventPresenterFactory.h"
#include "GUI/Experiment/ExperimentPresenterFactory.h"
#include "GUI/Instrument/InstrumentPresenterFactory.h"
#include "IReflBatchPresenter.h"
#include "IReflBatchView.h"
#include "ReflBatchPresenter.h"
#include "ReflRunsPresenterFactory.h"
#include "ReflSavePresenterFactory.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
class ReflBatchPresenterFactory {
public:
  ReflBatchPresenterFactory(
      RunsPresenterFactory runsPresenterFactory,
      EventPresenterFactory eventPresenterFactory,
      ExperimentPresenterFactory experimentPresenterFactory,
      InstrumentPresenterFactory instrumentPresenterFactory,
      SavePresenterFactory savePresenterFactory)
      : m_runsPresenterFactory(std::move(runsPresenterFactory)),
        m_eventPresenterFactory(std::move(eventPresenterFactory)),
        m_experimentPresenterFactory(std::move(experimentPresenterFactory)),
        m_instrumentPresenterFactory(std::move(instrumentPresenterFactory)),
        m_savePresenterFactory(std::move(savePresenterFactory)) {}

  std::unique_ptr<IReflBatchPresenter> make(IReflBatchView *view) {
    auto runsPresenter = m_runsPresenterFactory.make(view->runs());
    auto eventPresenter = m_eventPresenterFactory.make(view->eventHandling());
    auto experimentPresenter =
        m_experimentPresenterFactory.make(view->experiment());
    auto instrumentPresenter =
        m_instrumentPresenterFactory.make(view->instrument());
    auto savePresenter = m_savePresenterFactory.make(view->save());

    return std::make_unique<ReflBatchPresenter>(
        view, std::move(runsPresenter), std::move(eventPresenter),
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
}
}
#endif // MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTERFACTORY_H
