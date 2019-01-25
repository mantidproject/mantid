// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSTABPRESENTER_H

#include "IReflBatchPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class IRunsPresenter

IRunsPresenter is an interface which defines the functions any
reflectometry interface presenter needs to support.
*/
class IRunsPresenter {
public:
  virtual ~IRunsPresenter() = default;
  /// Accept a main presenter
  virtual void acceptMainPresenter(IReflBatchPresenter *mainPresenter) = 0;
  virtual void settingsChanged() = 0;
  virtual void notifyInstrumentChanged() = 0;

  virtual bool isAutoreducing() const = 0;
  virtual bool isProcessing() const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSTABPRESENTER_H */
