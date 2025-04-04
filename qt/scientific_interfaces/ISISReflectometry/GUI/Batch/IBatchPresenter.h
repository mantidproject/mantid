// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Batch/RowProcessingAlgorithm.h"
#include "GUI/Preview/ROIType.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "Reduction/Group.h"
#include "Reduction/ProcessingInstructions.h"

#include <memory>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class IMainWindowPresenter;

/** @class IBatchPresenter

IBatchPresenter is the interface defining the functions that the main
window presenter needs to implement. This interface is used by tab presenters to
request information from other tabs.
*/
class IBatchPresenter {
public:
  /// Destructor
  virtual ~IBatchPresenter() = default;

  virtual void acceptMainPresenter(IMainWindowPresenter *mainPresenter) = 0;
  virtual std::string initInstrumentList(const std::string &selectedInstrument = "") = 0;

  virtual void notifyPauseReductionRequested() = 0;
  virtual void notifyResumeReductionRequested() = 0;
  virtual void notifyResumeAutoreductionRequested() = 0;
  virtual void notifyPauseAutoreductionRequested() = 0;
  virtual void notifyAutoreductionCompleted() = 0;
  virtual void notifyChangeInstrumentRequested(const std::string &instrumentName) = 0;
  virtual void notifyInstrumentChanged(const std::string &instrumentName) = 0;
  virtual void notifyUpdateInstrumentRequested() = 0;
  virtual void notifySettingsChanged() = 0;
  virtual void notifySetRoundPrecision(int &precision) = 0;
  virtual void notifyResetRoundPrecision() = 0;
  virtual void notifyAnyBatchReductionResumed() = 0;
  virtual void notifyAnyBatchReductionPaused() = 0;
  virtual void notifyAnyBatchAutoreductionResumed() = 0;
  virtual void notifyAnyBatchAutoreductionPaused() = 0;
  virtual void notifyReductionPaused() = 0;
  virtual void notifyBatchLoaded() = 0;
  virtual void notifyRowContentChanged(Row &changedRow) = 0;
  virtual void notifyGroupNameChanged(Group &changedGroup) = 0;
  virtual void notifyRunsTransferred() = 0;
  virtual void notifyPreviewApplyRequested() = 0;

  /// Data processing check for all groups
  virtual bool isProcessing() const = 0;
  virtual bool isAutoreducing() const = 0;
  virtual bool isAnyBatchProcessing() const = 0;
  virtual bool isAnyBatchAutoreducing() const = 0;
  virtual bool isOverwriteBatchPrevented() const = 0;
  virtual bool discardChanges(std::string const &message) const = 0;
  virtual bool requestClose() const = 0;
  virtual int percentComplete() const = 0;
  virtual std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> rowProcessingProperties() const = 0;

  virtual bool isBatchUnsaved() const = 0;
  virtual void setBatchUnsaved() = 0;
  virtual void notifyChangesSaved() = 0;
  virtual Mantid::Geometry::Instrument_const_sptr instrument() const = 0;
  virtual std::string instrumentName() const = 0;

  virtual std::map<ROIType, ProcessingInstructions> getMatchingProcessingInstructionsForPreviewRow() const = 0;
  virtual boost::optional<ProcessingInstructions> getMatchingROIDetectorIDsForPreviewRow() const = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
