// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWVIEW_H

#include "IReflBatchView.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflMainWindowView

IReflMainWindowView is the interface defining the functions that the main
window view needs to implement. It is empty and not necessary at the moment, but
can be used in the future if widgets common to all tabs are added, for instance,
the help button.
*/
class ReflMainWindowSubscriber {
public:
  virtual void notifyHelpPressed() = 0;
  virtual void notifyNewBatchRequested() = 0;
  virtual void notifyCloseBatchRequested(int batchIndex) = 0;
  virtual ~ReflMainWindowSubscriber() = default;
};

class IReflMainWindowView {
public:
  virtual void subscribe(ReflMainWindowSubscriber *notifyee) = 0;
  virtual IReflBatchView *newBatch() = 0;
  virtual void removeBatch(int index) = 0;
  virtual std::vector<IReflBatchView *> batches() const = 0;
  virtual std::string runPythonAlgorithm(const std::string &pythonCode) = 0;
  virtual ~IReflMainWindowView() = default;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWVIEW_H */
