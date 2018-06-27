#ifndef MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H

#include <string>
#include "IReflBatchPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflEventTabPresenter

IReflEventPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Event' presenter

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

enum class SliceType { UniformEven, Uniform, Custom, LogValue };

class IReflEventTabPresenter {
public:
  virtual ~IReflEventTabPresenter() = default;
  /// Time-slicing values
  virtual std::string getTimeSlicingValues() const = 0;
  /// Time-slicing type
  virtual std::string getTimeSlicingType() const = 0;

  virtual void acceptMainPresenter(IReflBatchPresenter *mainPresenter) =0;
  virtual void onReductionPaused() = 0;
  virtual void onReductionResumed() = 0;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_IREFLEVENTPRESENTER_H */
