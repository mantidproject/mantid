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

/** @class IMainWindowView

IMainWindowView is the interface defining the functions that the main
window view needs to implement. It is empty and not necessary at the moment, but
can be used in the future if widgets common to all tabs are added, for instance,
the help button.
*/
class MainWindowSubscriber {
public:
  virtual void notifyHelpPressed(){};
  virtual void notifyNewBatchRequested(){};
  virtual void notifyCloseBatchRequested(int){};
  virtual ~MainWindowSubscriber() = default;
};

class IMainWindowView {
public:
  virtual void subscribe(MainWindowSubscriber *notifyee) = 0;
  virtual IBatchView *newBatch() = 0;
  virtual void removeBatch(int index) = 0;
  virtual std::vector<IBatchView *> batches() const = 0;

  virtual ~IMainWindowView() = default;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLWINDOWVIEW_H */
