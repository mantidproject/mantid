#ifndef MANTID_ISISREFLECTOMETRY_IREFLEVENTTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLEVENTTABPRESENTER_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;

/** @class IReflEventTabPresenter

IReflEventTabPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Event' tab presenter

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
