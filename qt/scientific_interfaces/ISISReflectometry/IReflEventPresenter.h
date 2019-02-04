// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;
class IReflEventTabPresenter;

/** @class IReflEventPresenter

IReflEventPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Event' presenter
*/

enum class SliceType { UniformEven, Uniform, Custom, LogValue };

class IReflEventPresenter {
public:
  virtual ~IReflEventPresenter(){};

  /// Time-slicing values
  virtual std::string getTimeSlicingValues() const = 0;
  /// Time-slicing type
  virtual std::string getTimeSlicingType() const = 0;

  virtual void acceptTabPresenter(IReflEventTabPresenter *tabPresenter) = 0;
  virtual void onReductionPaused() = 0;
  virtual void onReductionResumed() = 0;
  virtual void notifySliceTypeChanged(SliceType newSliceType) = 0;
  virtual void notifySettingsChanged() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H */
