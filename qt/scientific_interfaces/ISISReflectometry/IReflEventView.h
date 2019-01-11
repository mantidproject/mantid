// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLEVENTVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLEVENTVIEW_H

#include "DllConfig.h"
#include "IReflEventPresenter.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflEventView

IReflEventView is the base view class for the Reflectometry "Event Handling"
tab. It contains no QT specific functionality as that should be handled by a
subclass.
*/

class DLLExport IReflEventView {
public:
  IReflEventView(){};
  virtual ~IReflEventView(){};
  virtual IReflEventPresenter *getPresenter() const = 0;

  virtual std::string getLogValueTimeSlicingValues() const = 0;
  virtual std::string getCustomTimeSlicingValues() const = 0;
  virtual std::string getUniformTimeSlicingValues() const = 0;
  virtual std::string getUniformEvenTimeSlicingValues() const = 0;
  virtual std::string getLogValueTimeSlicingType() const = 0;

  virtual void enableSliceType(SliceType sliceType) = 0;
  virtual void disableSliceType(SliceType sliceType) = 0;
  virtual void enableSliceTypeSelection() = 0;
  virtual void disableSliceTypeSelection() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLEVENTVIEW_H */
