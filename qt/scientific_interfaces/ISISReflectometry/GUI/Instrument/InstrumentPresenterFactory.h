#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTERFACTORY_H
#include "DllConfig.h"
#include "IInstrumentView.h"
#include "IInstrumentPresenter.h"
#include "InstrumentPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class InstrumentPresenterFactory {
public:
  InstrumentPresenterFactory() {}

  std::unique_ptr<IInstrumentPresenter> make(IInstrumentView *view) {
    return std::make_unique<InstrumentPresenter>(view);
  }
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTERFACTORY_H
