#ifndef MANTID_ISISREFLECTOMETRY_IREFLEVENTVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLEVENTVIEW_H

#include "../../DllConfig.h"
#include <string>
#include "IEventPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
IEventView is the base view class for the Reflectometry "Event Handling"
tab. It contains no QT specific functionality as that should be handled by a
subclass.

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL EventTabViewSubscriber {
public:
  virtual void notifySliceTypeChanged(SliceType newSliceType) = 0;
  virtual void notifyUniformSliceCountChanged(int sliceCount) = 0;
  virtual void notifyUniformSecondsChanged(double sliceLengthInSeconds) = 0;
  virtual void
  notifyCustomSliceValuesChanged(std::string pythonListOfSliceTimes) = 0;
  virtual void
  notifyLogSliceBreakpointsChanged(std::string logValueBreakpoints) = 0;
  virtual void notifyLogBlockNameChanged(std::string blockName) = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IEventView {
public:
  virtual void subscribe(EventTabViewSubscriber *notifyee) = 0;
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
}
}
#endif // MANTID_ISISREFLECTOMETRY_IREFLEVENTVIEW_H
