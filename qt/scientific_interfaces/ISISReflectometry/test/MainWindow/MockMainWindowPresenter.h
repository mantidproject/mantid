// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  MOCK_CONST_METHOD0(isWarnProcessAllChecked, bool());
  MOCK_CONST_METHOD0(isWarnProcessPartialGroupChecked, bool());
  MOCK_CONST_METHOD0(isWarnDiscardChangesChecked, bool());
  MOCK_CONST_METHOD0(isRoundChecked, bool());
  MOCK_CONST_METHOD0(getRoundPrecision, int &());
  MOCK_CONST_METHOD0(roundPrecision, boost::optional<int>());
  MOCK_METHOD0(isCloseEventPrevented, bool());
  MOCK_CONST_METHOD1(isCloseBatchPrevented, bool(int));
  MOCK_CONST_METHOD1(isOverwriteBatchPrevented, bool(int));
  MOCK_CONST_METHOD1(isOverwriteBatchPrevented, bool(IBatchPresenter const *));
  MOCK_CONST_METHOD0(isProcessAllPrevented, bool());
  MOCK_CONST_METHOD0(isProcessPartialGroupPrevented, bool());
  MOCK_CONST_METHOD1(isBatchUnsaved, bool(int));
  MOCK_CONST_METHOD0(isAnyBatchUnsaved, bool());
  MOCK_CONST_METHOD0(notifyOptionsChanged, void());
  MOCK_METHOD0(notifyAnyBatchAutoreductionResumed, void());
  MOCK_METHOD0(notifyAnyBatchAutoreductionPaused, void());
  MOCK_METHOD0(notifyAnyBatchReductionResumed, void());
  MOCK_METHOD0(notifyAnyBatchReductionPaused, void());
  MOCK_METHOD1(notifyChangeInstrumentRequested, void(std::string const &));
  MOCK_METHOD0(notifyCloseEvent, void());
  MOCK_METHOD0(notifyUpdateInstrumentRequested, void());
  MOCK_CONST_METHOD0(instrument, Mantid::Geometry::Instrument_const_sptr());
  MOCK_CONST_METHOD0(instrumentName, std::string());
  MOCK_CONST_METHOD1(discardChanges, bool(std::string const &));

  ~MockMainWindowPresenter() override {};
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
