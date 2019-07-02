// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSPRESENTER_H

#include "Reduction/RunsTable.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IBatchPresenter;

/** @class IRunsPresenter

IRunsPresenter is an interface which defines the functions any
reflectometry interface presenter needs to support.
*/
class IRunsPresenter {
public:
  virtual ~IRunsPresenter() = default;
  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
  virtual RunsTable const &runsTable() const = 0;
  virtual RunsTable &mutableRunsTable() = 0;

  virtual void notifyInstrumentChanged(std::string const &instrumentName) = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyReductionPaused() = 0;
  virtual void notifyRowStateChanged() = 0;
  virtual void notifyRowStateChanged(boost::optional<Item const &> item) = 0;
  virtual void notifyRowOutputsChanged(boost::optional<Item const &> item) = 0;
  virtual void notifyRowOutputsChanged() = 0;

  virtual void reductionPaused() = 0;
  virtual void reductionResumed() = 0;
  virtual bool resumeAutoreduction() = 0;
  virtual void autoreductionPaused() = 0;
  virtual void autoreductionCompleted() = 0;
  virtual void autoreductionResumed() = 0;
  virtual void instrumentChanged(std::string const &instrumentName) = 0;
  virtual void settingsChanged() = 0;

  virtual bool isProcessing() const = 0;
  virtual bool isAutoreducing() const = 0;
  virtual int percentComplete() const = 0;
  virtual void notifySearchComplete() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSPRESENTER_H */
