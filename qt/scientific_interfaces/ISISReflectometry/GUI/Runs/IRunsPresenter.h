// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Reduction/RunsTable.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class IBatchPresenter;

/** @class IRunsPresenter

IRunsPresenter is an interface which defines the functions any
reflectometry interface presenter needs to support.
*/
class IRunsPresenter {
public:
  virtual ~IRunsPresenter() = default;
  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual std::string initInstrumentList(const std::string &selectedInstrument = "") = 0;
  virtual RunsTable const &runsTable() const = 0;
  virtual RunsTable &mutableRunsTable() = 0;

  virtual bool notifyChangeInstrumentRequested(std::string const &instrumentName) = 0;
  virtual void notifyResumeReductionRequested() = 0;
  virtual void notifyPauseReductionRequested() = 0;
  virtual void notifyRowStateChanged() = 0;
  virtual void notifyRowStateChanged(boost::optional<Item const &> item) = 0;
  virtual void notifyRowModelChanged(boost::optional<Item const &> item) = 0;
  virtual void notifyRowModelChanged() = 0;
  virtual void notifyBatchLoaded() = 0;

  virtual void notifyReductionPaused() = 0;
  virtual void notifyReductionResumed() = 0;
  virtual bool resumeAutoreduction() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void autoreductionCompleted() = 0;
  virtual void notifyAutoreductionResumed() = 0;
  virtual void notifyAnyBatchReductionResumed() = 0;
  virtual void notifyAnyBatchReductionPaused() = 0;
  virtual void notifyAnyBatchAutoreductionResumed() = 0;
  virtual void notifyAnyBatchAutoreductionPaused() = 0;
  virtual void notifyInstrumentChanged(std::string const &instrumentName) = 0;
  virtual void notifyTableChanged() = 0;
  virtual void notifyRowContentChanged(Row &changedRow) = 0;
  virtual void notifyGroupNameChanged(Group &changedGroup) = 0;
  virtual void settingsChanged() = 0;
  virtual void notifyChangesSaved() = 0;
  virtual bool hasUnsavedChanges() const = 0;

  virtual bool isAnyBatchProcessing() const = 0;
  virtual bool isAnyBatchAutoreducing() const = 0;

  virtual bool isProcessing() const = 0;
  virtual bool isAutoreducing() const = 0;
  virtual int percentComplete() const = 0;
  virtual void setRoundPrecision(int &precision) = 0;
  virtual void resetRoundPrecision() = 0;
  virtual void notifySearchComplete() = 0;

  virtual std::string instrumentName() const = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
