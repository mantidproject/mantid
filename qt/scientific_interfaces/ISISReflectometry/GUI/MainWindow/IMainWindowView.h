// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLWINDOWVIEW_H

#include "GUI/Batch/IBatchView.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class IMainWindowView

IMainWindowView is the interface defining the functions that the main
window view needs to implement. It is empty and not necessary at the moment, but
can be used in the future if widgets common to all tabs are added, for instance,
the help button.
*/
class MainWindowSubscriber {
public:
  virtual void notifyHelpPressed() = 0;
  virtual void notifyNewBatchRequested() = 0;
  virtual void notifyCloseBatchRequested(int) = 0;
  virtual void notifySaveBatchRequested(int) = 0;
  virtual void notifyLoadBatchRequested(int) = 0;
  virtual void notifyShowOptionsRequested() = 0;
  virtual void notifyShowSlitCalculatorRequested() = 0;
  virtual ~MainWindowSubscriber() = default;
};

class IMainWindowView {
public:
  virtual void subscribe(MainWindowSubscriber *notifyee) = 0;
  virtual IBatchView *newBatch() = 0;
  virtual void removeBatch(int index) = 0;
  virtual std::vector<IBatchView *> batches() const = 0;
  virtual void disableSaveAndLoadBatch() = 0;
  virtual void enableSaveAndLoadBatch() = 0;

  virtual ~IMainWindowView() = default;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLWINDOWVIEW_H */
