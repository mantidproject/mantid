// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IMAINWINDOWPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IMAINWINDOWPRESENTER_H

#include "IMainWindowView.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class IMainWindowPresenter

IMainWindowPresenter is the interface defining the functions that the main
window presenter needs to implement. This interface is used by tab presenters to
request information from other tabs.
*/
class IMainWindowPresenter {
public:
  virtual bool isAnyBatchProcessing() const = 0;
  virtual bool isAnyBatchAutoreducing() const = 0;
  virtual void notifyAutoreductionResumed() = 0;
  virtual void notifyAutoreductionPaused() = 0;
  virtual ~IMainWindowPresenter() = default;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IMAINWINDOWPRESENTER_H */
