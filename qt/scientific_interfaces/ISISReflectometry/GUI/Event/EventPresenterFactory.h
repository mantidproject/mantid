#ifndef MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
#include "DllConfig.h"
#include "IEventView.h"
#include "IEventPresenter.h"
#include "EventPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class EventPresenterFactory {
public:
  std::unique_ptr<IEventPresenter> make(IEventView *view) {
    return std::make_unique<EventPresenter>(view);
  }
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
