// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSTABLEPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSTABLEPRESENTER_H

#include "GUI/Batch/IBatchPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class IRunsPresenter;
class RunsTable;
class ReductionJobs;
class Item;

/** @class IRunsTablePresenter

IRunsTablePresenter is an interface which defines the functions any
reflectometry interface presenter needs to support.
*/
class IRunsTablePresenter {
public:
  virtual ~IRunsTablePresenter() = default;
  virtual void acceptMainPresenter(IRunsPresenter *mainPresenter) = 0;
  virtual RunsTable const &runsTable() const = 0;
  virtual RunsTable &mutableRunsTable() = 0;

  virtual void notifyRowStateChanged() = 0;
  virtual void notifyRowStateChanged(boost::optional<Item const &> item) = 0;

  virtual void notifyRowOutputsChanged() = 0;
  virtual void notifyRowOutputsChanged(boost::optional<Item const &> item) = 0;
  virtual void notifyRemoveAllRowsAndGroupsRequested() = 0;

  virtual void mergeAdditionalJobs(ReductionJobs const &jobs) = 0;

  virtual void notifyReductionPaused() = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual void notifyAutoreductionResumed() = 0;
  virtual void notifyAnyBatchReductionPaused() = 0;
  virtual void notifyAnyBatchReductionResumed() = 0;
  virtual void notifyAnyBatchAutoreductionPaused() = 0;
  virtual void notifyAnyBatchAutoreductionResumed() = 0;
  virtual void notifyInstrumentChanged(std::string const &instrumentName) = 0;
  virtual void setTablePrecision(int &precision) = 0;
  virtual void resetTablePrecision() = 0;
  virtual void settingsChanged() = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSTABLEPRESENTER_H */
