#ifndef MANTID_CUSTOMINTERFACES_IREFLEVENTVIEW_H
#define MANTID_CUSTOMINTERFACES_IREFLEVENTVIEW_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IReflEventPresenter;

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
  /// Constructor
  IReflEventView(){};
  /// Destructor
  virtual ~IReflEventView(){};
  /// Returns the presenter managing this view
  virtual IReflEventPresenter *getPresenter() const = 0;

  /// Slice type enums
  enum class SliceType { UniformEven, Uniform, Custom, LogValue };

  virtual std::string getTimeSlicingValues() const = 0;
  virtual std::string getTimeSlicingType() const = 0;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_IREFLEVENTVIEW_H */
