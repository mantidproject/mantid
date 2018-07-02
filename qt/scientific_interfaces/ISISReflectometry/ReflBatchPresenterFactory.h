#ifndef MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTERFACTORY_H
#include "DllConfig.h"
#include "IReflBatchView.h"
#include "IReflBatchPresenter.h"
#include "ReflRunsPresenterFactory.h"
#include "ReflSavePresenterFactory.h"
#include "ReflSettingsPresenterFactory.h"
#include "GUI/Experiment/ExperimentPresenterFactory.h"
#include "GUI/Event/EventPresenterFactory.h"
#include "ReflBatchPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
class ReflBatchPresenterFactory {
public:
  ReflBatchPresenterFactory(
      RunsPresenterFactory runsPresenterFactory,
      EventPresenterFactory eventPresenterFactory,
      ExperimentPresenterFactory experimentPresenterFactory,
      SettingsPresenterFactory settingsPresenterFactory,
      SavePresenterFactory savePresenterFactory)
      : m_runsPresenterFactory(std::move(runsPresenterFactory)),
        m_eventPresenterFactory(std::move(eventPresenterFactory)),
        m_experimentPresenterFactory(std::move(experimentPresenterFactory)),
        m_settingsPresenterFactory(std::move(settingsPresenterFactory)),
        m_savePresenterFactory(std::move(savePresenterFactory)) {}

  std::unique_ptr<IReflBatchPresenter> make(IReflBatchView *view) {
    auto runsPresenter = m_runsPresenterFactory.make(view->runs());
    auto eventPresenter = m_eventPresenterFactory.make(view->eventHandling());
    auto experimentPresenter =
        m_experimentPresenterFactory.make(view->experiment());
    auto settingsPresenter = m_settingsPresenterFactory.make(view->settings());
    auto savePresenter = m_savePresenterFactory.make(view->save());

    return std::make_unique<ReflBatchPresenter>(
        view, std::move(runsPresenter), std::move(eventPresenter),
        std::move(experimentPresenter), std::move(settingsPresenter),
        std::move(savePresenter));
  }

private:
  RunsPresenterFactory m_runsPresenterFactory;
  EventPresenterFactory m_eventPresenterFactory;
  ExperimentPresenterFactory m_experimentPresenterFactory;
  SettingsPresenterFactory m_settingsPresenterFactory;
  SavePresenterFactory m_savePresenterFactory;
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_REFLBATCHPRESENTERFACTORY_H
