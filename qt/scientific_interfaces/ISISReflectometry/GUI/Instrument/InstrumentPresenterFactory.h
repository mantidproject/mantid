#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTERFACTORY_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTERFACTORY_H
#include "../../Reduction/Instrument.h"
#include "DllConfig.h"
#include "IInstrumentPresenter.h"
#include "IInstrumentView.h"
#include "InstrumentPresenter.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class InstrumentPresenterFactory {
public:
  InstrumentPresenterFactory() {}

  std::unique_ptr<IInstrumentPresenter> make(IInstrumentView *view) {
    return std::make_unique<InstrumentPresenter>(view, makeModel());
  }

private:
  // TODO get defaults from algorithm
  Instrument makeModel() {
    auto wavelengthRange = RangeInLambda(0.0, 0.0);
    auto monitorCorrections = MonitorCorrections(
        0, true, RangeInLambda(0.0, 0.0), RangeInLambda(0.0, 0.0));
    auto detectorCorrections =
        DetectorCorrections(false, DetectorCorrectionType::VerticalShift);
    return Instrument(wavelengthRange, monitorCorrections, detectorCorrections);
  }
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTERFACTORY_H
