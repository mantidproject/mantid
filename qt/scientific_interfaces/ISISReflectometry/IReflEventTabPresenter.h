// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLEVENTTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLEVENTTABPRESENTER_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;

/** @class IReflEventTabPresenter

IReflEventTabPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Event' tab presenter
*/
class IReflEventTabPresenter {
public:
  virtual ~IReflEventTabPresenter(){};
  /// Time-slicing values
  virtual std::string getTimeSlicingValues(int group) const = 0;
  /// Time-slicing type
  virtual std::string getTimeSlicingType(int group) const = 0;

  virtual void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) = 0;
  virtual void settingsChanged(int group) = 0;
  virtual void onReductionPaused(int group) = 0;
  virtual void onReductionResumed(int group) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLEVENTTABPRESENTER_H */
