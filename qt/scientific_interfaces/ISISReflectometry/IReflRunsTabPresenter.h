#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSTABPRESENTER_H

#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;

/** @class IReflRunsTabPresenter

IReflRunsTabPresenter is an interface which defines the functions any
reflectometry interface presenter needs to support.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class IReflRunsTabPresenter {
public:
  virtual ~IReflRunsTabPresenter(){};
  /// Accept a main presenter
  virtual void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) = 0;
  virtual void settingsChanged(int group) = 0;

  enum Flag {
    SearchFlag,
    StartAutoreductionFlag,
    TimerEventFlag,
    ICATSearchCompleteFlag,
    TransferFlag,
    InstrumentChangedFlag,
    GroupChangedFlag
  };

  // Tell the presenter something happened
  virtual void notify(IReflRunsTabPresenter::Flag flag) = 0;
  // Determine whether to start a new autoreduction
  virtual bool startNewAutoreduction() const = 0;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSTABPRESENTER_H */
