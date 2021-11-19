// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IMainWindowView.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class IBatchPresenter;

/** @class IMainWindowPresenter

IMainWindowPresenter is the interface defining the functions that the main
window presenter needs to implement. This interface is used by tab presenters to
request information from other tabs.
*/
class IMainWindowPresenter {
public:
  virtual bool isAnyBatchProcessing() const = 0;
  virtual bool isAnyBatchAutoreducing() const = 0;
  virtual bool isWarnProcessAllChecked() const = 0;
  virtual bool isWarnProcessPartialGroupChecked() const = 0;
  virtual bool isProcessAllPrevented() const = 0;
  virtual bool isProcessPartialGroupPrevented() const = 0;
  virtual bool isRoundChecked() const = 0;
  virtual int &getRoundPrecision() const = 0;
  virtual std::optional<int> roundPrecision() const = 0;
  virtual bool isCloseEventPrevented() = 0;
  virtual bool isCloseBatchPrevented(int batchIndex) const = 0;
  virtual bool isOverwriteBatchPrevented(int tabIndex) const = 0;
  virtual bool isOverwriteBatchPrevented(IBatchPresenter const *batchPresenter) const = 0;
  virtual bool isBatchUnsaved(int batchIndex) const = 0;
  virtual bool isAnyBatchUnsaved() const = 0;
  virtual void notifyAnyBatchAutoreductionResumed() = 0;
  virtual void notifyAnyBatchAutoreductionPaused() = 0;
  virtual void notifyAnyBatchReductionResumed() = 0;
  virtual void notifyAnyBatchReductionPaused() = 0;
  virtual void notifyChangeInstrumentRequested(std::string const &instrumentName) = 0;
  virtual void notifyCloseEvent() = 0;
  virtual void notifyUpdateInstrumentRequested() = 0;
  virtual Mantid::Geometry::Instrument_const_sptr instrument() const = 0;
  virtual std::string instrumentName() const = 0;
  virtual bool discardChanges(std::string const &message) const = 0;
  virtual ~IMainWindowPresenter() = default;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
