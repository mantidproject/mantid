// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTER_H

#include "../../Reduction/Instrument.h"
#include "Common/DllConfig.h"
#include "IInstrumentPresenter.h"
#include "IInstrumentView.h"
#include "InstrumentOptionDefaults.h"
#include "MantidGeometry/Instrument_fwd.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/** @class InstrumentPresenter

InstrumentPresenter is a presenter class for the widget 'Instrument' in the
ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentPresenter
    : public InstrumentViewSubscriber,
      public IInstrumentPresenter {
public:
  InstrumentPresenter(
      IInstrumentView *view, Instrument instrument,
      std::unique_ptr<IInstrumentOptionDefaults> instrumentDefaults =
          std::make_unique<InstrumentOptionDefaults>());
  Instrument const &instrument() const override;

  // IInstrumentPresenver overrides
  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  void reductionPaused() override;
  void reductionResumed() override;
  void autoreductionPaused() override;
  void autoreductionResumed() override;
  void instrumentChanged(std::string const &instrumentName) override;

  // InstrumentViewSubscriber overrides
  void notifySettingsChanged() override;
  void notifyRestoreDefaultsRequested() override;

protected:
  std::unique_ptr<IInstrumentOptionDefaults> m_instrumentDefaults;

private:
  IInstrumentView *m_view;
  Instrument m_model;
  IBatchPresenter *m_mainPresenter;

  void restoreDefaults();
  boost::optional<RangeInLambda> wavelengthRangeFromView();
  boost::optional<RangeInLambda> monitorBackgroundRangeFromView();
  boost::optional<RangeInLambda> monitorIntegralRangeFromView();
  MonitorCorrections monitorCorrectionsFromView();
  DetectorCorrectionType detectorCorrectionTypeFromView();
  DetectorCorrections detectorCorrectionsFromView();
  void updateModelFromView();
  void updateViewFromModel();
  void updateWidgetEnabledState();
  void updateWidgetValidState();
  bool isProcessing() const;
  bool isAutoreducing() const;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTER_H
