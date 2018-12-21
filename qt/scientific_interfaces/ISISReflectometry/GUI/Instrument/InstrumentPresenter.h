// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTER_H

#include "../../DllConfig.h"
#include "../../Reduction/Instrument.h"
#include "IInstrumentPresenter.h"
#include "IInstrumentView.h"
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
  // TODO Inject the Instrument model into the constructor.
  InstrumentPresenter(IInstrumentView *view, Instrument instrument);
  Instrument const &instrument() const;

  // IInstrumentPresenver overrides
  void reductionPaused() override;
  void reductionResumed() override;
  void setInstrumentName(std::string const &instrumentName) override;

  // InstrumentViewSubscriber overrides
  void notifySettingsChanged() override;

private:
  IInstrumentView *m_view;
  Instrument m_model;

  boost::optional<RangeInLambda> wavelengthRangeFromView();
  boost::optional<RangeInLambda> monitorBackgroundRangeFromView();
  boost::optional<RangeInLambda> monitorIntegralRangeFromView();
  MonitorCorrections monitorCorrectionsFromView();
  DetectorCorrectionType detectorCorrectionTypeFromView();
  DetectorCorrections detectorCorrectionsFromView();
  void updateModelFromView();
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_INSTRUMENTPRESENTER_H
