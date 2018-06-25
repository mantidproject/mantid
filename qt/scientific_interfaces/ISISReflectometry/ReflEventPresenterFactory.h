#ifndef MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
#include "DllConfig.h"
#include "IReflEventTabView.h"
#include "IReflEventTabPresenter.h"
#include "ReflEventTabPresenter.h"
#include "Presenters/BatchPresenterFactory.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class EventPresenterFactory {
public:
  std::unique_ptr<IReflEventTabPresenter> make(IReflEventTabView *view) {
    return std::make_unique<ReflEventTabPresenter>(view);
  }
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
