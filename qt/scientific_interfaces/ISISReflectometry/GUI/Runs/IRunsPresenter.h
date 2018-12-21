// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSPRESENTER_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IReflBatchPresenter;

/** @class IRunsPresenter

IRunsPresenter is an interface which defines the functions any
reflectometry interface presenter needs to support.
*/
class IRunsPresenter {
public:
  virtual ~IRunsPresenter() = default;
  virtual void acceptMainPresenter(IReflBatchPresenter *mainPresenter) = 0;

  virtual void settingsChanged() = 0;
  virtual void notifyInstrumentChanged(std::string const &instrumentName) = 0;
  virtual void notifyReductionResumed() = 0;
  virtual void notifyReductionPaused() = 0;

  virtual void setInstrumentName(std::string const &instrumentName) = 0;

  virtual bool isAutoreducing() const = 0;
  virtual bool isProcessing() const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSPRESENTER_H */
