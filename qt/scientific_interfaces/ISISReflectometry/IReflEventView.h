#ifndef MANTID_ISISREFLECTOMETRY_IREFLEVENTVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLEVENTVIEW_H

#include "DllConfig.h"
#include <string>
#include "IReflEventPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflEventView

IReflEventView is the base view class for the Reflectometry "Event Handling"
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
}
}
#endif /* MANTID_ISISREFLECTOMETRY_IREFLEVENTVIEW_H */
