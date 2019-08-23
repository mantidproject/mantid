// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "../../../ISISReflectometry/GUI/MainWindow/IMainWindowPresenter.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class MockMainWindowPresenter : public IMainWindowPresenter {
public:
  MOCK_METHOD1(settingsChanged, void(int));
  MOCK_CONST_METHOD0(isAnyBatchProcessing, bool());
  MOCK_CONST_METHOD0(isAnyBatchAutoreducing, bool());
  MOCK_METHOD0(notifyAnyBatchAutoreductionResumed, void());
  MOCK_METHOD0(notifyAnyBatchAutoreductionPaused, void());
  MOCK_METHOD0(notifyAnyBatchReductionResumed, void());
  MOCK_METHOD0(notifyAnyBatchReductionPaused, void());
  MOCK_METHOD1(notifyChangeInstrumentRequested, void(std::string const &));
  MOCK_METHOD0(notifyUpdateInstrumentRequested, void());
  MOCK_CONST_METHOD0(instrument, Mantid::Geometry::Instrument_const_sptr());
  MOCK_CONST_METHOD0(instrumentName, std::string());

  ~MockMainWindowPresenter() override{};
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
