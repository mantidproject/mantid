// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSTABPRESENTER_H

#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;

/** @class IReflRunsTabPresenter

IReflRunsTabPresenter is an interface which defines the functions any
reflectometry interface presenter needs to support.
*/
class IReflRunsTabPresenter {
public:
  virtual ~IReflRunsTabPresenter(){};
  /// Accept a main presenter
  virtual void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) = 0;
  virtual void settingsChanged(int group) = 0;

  enum Flag {
    SearchFlag,
    StartAutoreductionFlag,
    PauseAutoreductionFlag,
    TimerEventFlag,
    ICATSearchCompleteFlag,
    TransferFlag,
    InstrumentChangedFlag,
    GroupChangedFlag,
    StartMonitorFlag,
    StopMonitorFlag,
    StartMonitorCompleteFlag
  };

  // Tell the presenter something happened
  virtual void notify(IReflRunsTabPresenter::Flag flag) = 0;
  virtual bool isAutoreducing(int group) const = 0;
  virtual bool isAutoreducing() const = 0;
  virtual bool isProcessing(int group) const = 0;
  virtual bool isProcessing() const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSTABPRESENTER_H */
