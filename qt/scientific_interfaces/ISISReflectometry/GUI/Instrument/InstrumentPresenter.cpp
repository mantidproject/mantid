#include "InstrumentPresenter.h"
#include "../../Reduction/ValidatePerThetaDefaults.h"
#include "../../Reduction/ParseReflectometryStrings.h"
#include "PerThetaDefaultsTableValidator.h"

namespace MantidQt {
namespace CustomInterfaces {

InstrumentPresenter::InstrumentPresenter(IInstrumentView *view)
    : m_view(view) {
  m_view->subscribe(this);
  notifySettingsChanged();
}

void InstrumentPresenter::notifySettingsChanged() {
  //auto validationResult = updateModelFromView();
  //showValidationResult(validationResult);
}
}
}
