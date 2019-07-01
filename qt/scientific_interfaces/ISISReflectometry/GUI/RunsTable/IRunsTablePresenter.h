// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSTABLEPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSTABLEPRESENTER_H

#include "GUI/Batch/IBatchPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"

namespace MantidQt {
namespace CustomInterfaces {

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

  virtual void reductionPaused() = 0;
  virtual void reductionResumed() = 0;
  virtual void autoreductionPaused() = 0;
  virtual void autoreductionResumed() = 0;
  virtual void instrumentChanged(std::string const &instrumentName) = 0;
  virtual void settingsChanged() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSTABLEPRESENTER_H */
