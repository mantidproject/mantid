#ifndef MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H

#include "DllConfig.h"
#include "IReflEventTabPresenter.h"
#include "IReflBatchPresenter.h"
#include "IReflEventTabView.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class ReflEventTabPresenter

ReflEventTabPresenter is a presenter class for the widget 'Event' in the
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
    : public IReflEventTabPresenter, public EventTabViewSubscriber {
public:
  /// Constructor
  ReflEventTabPresenter(IReflEventTabView *view);
  /// Returns time-slicing values
  std::string getTimeSlicingValues() const override;
  /// Returns time-slicing type
  std::string getTimeSlicingType() const override;

  void onReductionPaused() override;
  void onReductionResumed() override;
  void notifySliceTypeChanged(SliceType newSliceType) override;
  void notifySettingsChanged() override;

  void acceptMainPresenter(IReflBatchPresenter *mainPresenter) override;

private:
  std::string logFilterAndSliceValues(std::string const &slicingValues,
                                      std::string const &logFilter) const;
  /// The view we are managing
  IReflEventTabView *m_view;
  IReflBatchPresenter *m_mainPresenter;
  SliceType m_sliceType;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H */
