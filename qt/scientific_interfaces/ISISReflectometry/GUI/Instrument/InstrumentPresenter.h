// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../Reduction/Instrument.h"
#include "Common/DllConfig.h"
#include "GUI/Common/IFileHandler.h"
#include "GUI/Common/IReflMessageHandler.h"
#include "IInstrumentPresenter.h"
#include "IInstrumentView.h"
#include "InstrumentOptionDefaults.h"
#include "MantidGeometry/Instrument_fwd.h"
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class InstrumentPresenter

InstrumentPresenter is a presenter class for the widget 'Instrument' in the
ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentPresenter : public InstrumentViewSubscriber,
                                                           public IInstrumentPresenter {
public:
  InstrumentPresenter(
      IInstrumentView *view, Instrument instrument, IFileHandler *fileHandler, IReflMessageHandler *messageHandler,
      std::unique_ptr<IInstrumentOptionDefaults> instrumentDefaults = std::make_unique<InstrumentOptionDefaults>());
  Instrument const &instrument() const override;

  // IInstrumentPresenter overrides
  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  void notifyReductionPaused() override;
  void notifyReductionResumed() override;
  void notifyAutoreductionPaused() override;
  void notifyAutoreductionResumed() override;
  void notifyInstrumentChanged(std::string const &instrumentName) override;
  void restoreDefaults() override;

  // InstrumentViewSubscriber overrides
  void notifySettingsChanged() override;
  void notifyRestoreDefaultsRequested() override;
  void notifyBrowseToCalibrationFileRequested() override;

protected:
  std::unique_ptr<IInstrumentOptionDefaults> m_instrumentDefaults;

private:
  IInstrumentView *m_view;
  Instrument m_model;
  IBatchPresenter *m_mainPresenter;
  IFileHandler *m_fileHandler;
  IReflMessageHandler *m_messageHandler;

  std::optional<RangeInLambda> wavelengthRangeFromView();
  std::optional<RangeInLambda> monitorBackgroundRangeFromView();
  std::optional<RangeInLambda> monitorIntegralRangeFromView();
  MonitorCorrections monitorCorrectionsFromView();
  DetectorCorrectionType detectorCorrectionTypeFromView();
  DetectorCorrections detectorCorrectionsFromView();
  std::string calibrationFilePathFromView();
  void updateModelFromView();
  void updateViewFromModel();
  void updateWidgetEnabledState();
  void updateWidgetValidState();
  void updateCalibrationFileValidState(const std::string &calibrationFilePath);
  bool isProcessing() const;
  bool isAutoreducing() const;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
