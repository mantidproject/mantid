#ifndef MANTID_ISISREFLECTOMETRY_REFLSAVEPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_REFLSAVEPRESENTERFACTORY_H
#include "DllConfig.h"
#include "IReflSaveTabView.h"
#include "IReflSaveTabPresenter.h"
#include "ReflSaveTabPresenter.h"
#include "ReflAsciiSaver.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class SavePresenterFactory {
public:
  std::unique_ptr<IReflSaveTabPresenter> make(IReflSaveTabView *view) {
    return Mantid::Kernel::make_unique<ReflSaveTabPresenter>(
        view, Mantid::Kernel::make_unique<ReflAsciiSaver>());
  }
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_REFLSETTINGSPRESENTERFACTORY_H
