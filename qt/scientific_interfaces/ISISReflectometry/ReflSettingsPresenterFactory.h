#ifndef MANTID_ISISREFLECTOMETRY_REFLSETTINGSPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLSETTINGSPRESENTERFACTORY_H
#include "DllConfig.h"
#include "IReflSettingsTabView.h"
#include "IReflSettingsTabPresenter.h"
#include "ReflSettingsTabPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class SettingsPresenterFactory {
public:
  std::unique_ptr<IReflSettingsTabPresenter> make(IReflSettingsTabView *view) {
    return std::make_unique<ReflSettingsTabPresenter>(view);
  }
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_REFLSETTINGSPRESENTERFACTORY_H
