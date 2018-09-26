#ifndef MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
#include "DllConfig.h"
#include "EventPresenter.h"
#include "IEventPresenter.h"
#include "IEventView.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class EventPresenterFactory {
public:
  std::unique_ptr<IEventPresenter> make(IEventView *view) {
    return std::make_unique<EventPresenter>(view);
  }
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTERFACTORY_H
