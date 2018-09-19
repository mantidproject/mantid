#ifndef MANTID_ISISREFLECTOMETRY_REFLEVENTTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLEVENTTABPRESENTER_H

#include "DllConfig.h"
#include "IReflEventTabPresenter.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflMainWindowPresenter;
class IReflEventPresenter;

/** @class ReflEventTabPresenter

ReflEventTabPresenter is a presenter class for the tab 'Event' in the
ISIS Reflectometry Interface.

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
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflEventTabPresenter
    : public IReflEventTabPresenter {
public:
  /// Constructor
  ReflEventTabPresenter(std::vector<IReflEventPresenter *> presenters);
  /// Destructor
  ~ReflEventTabPresenter() override;

  /// Returns time-slicing values
  std::string getTimeSlicingValues(int group) const override;
  /// Return time-slicing type
  std::string getTimeSlicingType(int group) const override;

  void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) override;
  void settingsChanged(int group) override;
  void onReductionResumed(int group) override;
  void onReductionPaused(int group) override;
  void passSelfToChildren(std::vector<IReflEventPresenter *> const &children);

private:
  /// The presenters for each group as a vector
  std::vector<IReflEventPresenter *> m_eventPresenters;
  IReflMainWindowPresenter *m_mainPresenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_REFLEVENTTABPRESENTER_H */
