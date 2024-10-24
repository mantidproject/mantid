// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

enum class SliceType { None, UniformEven, Uniform, Custom, LogValue };

class MANTIDQT_ISISREFLECTOMETRY_DLL EventViewSubscriber {
public:
  virtual void notifySliceTypeChanged(SliceType newSliceType) = 0;
  virtual void notifyUniformSliceCountChanged(int sliceCount) = 0;
  virtual void notifyUniformSecondsChanged(double sliceLengthInSeconds) = 0;
  virtual void notifyCustomSliceValuesChanged(std::string pythonListOfSliceTimes) = 0;
  virtual void notifyLogSliceBreakpointsChanged(std::string logValueBreakpoints) = 0;
  virtual void notifyLogBlockNameChanged(std::string blockName) = 0;
};

/** @class IEventView

IEventView is the base view class for the tab "Event" in the
Reflectometry Interface. It contains no QT specific functionality as that should
be handled by a subclass.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL IEventView {
public:
  virtual void subscribe(EventViewSubscriber *notifyee) = 0;
  virtual ~IEventView() = default;

  virtual std::string logBlockName() const = 0;
  virtual std::string logBreakpoints() const = 0;
  virtual std::string customBreakpoints() const = 0;
  virtual int uniformSliceCount() const = 0;
  virtual double uniformSliceLength() const = 0;

  virtual void showCustomBreakpointsInvalid() = 0;
  virtual void showCustomBreakpointsValid() = 0;
  virtual void showLogBreakpointsInvalid() = 0;
  virtual void showLogBreakpointsValid() = 0;

  virtual void enableSliceType(SliceType sliceType) = 0;
  virtual void disableSliceType(SliceType sliceType) = 0;
  virtual void enableSliceTypeSelection() = 0;
  virtual void disableSliceTypeSelection() = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
