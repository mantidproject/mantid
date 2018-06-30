#ifndef MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H

#include "DllConfig.h"
#include "IReflEventPresenter.h"
#include "IReflEventTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflEventView;

/** @class ReflEventPresenter

ReflEventPresenter is a presenter class for the widget 'Event' in the
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
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflEventPresenter
    : public IReflEventPresenter {
public:
  /// Constructor
  ReflEventPresenter(IReflEventView *view, int group);
  /// Destructor
  ~ReflEventPresenter() override;

  /// Returns time-slicing values
  std::string getTimeSlicingValues() const override;
  /// Returns time-slicing type
  std::string getTimeSlicingType() const override;

  void onReductionPaused() override;
  void onReductionResumed() override;
  void notifySliceTypeChanged(SliceType newSliceType) override;
  void notifySettingsChanged() override;

  void acceptTabPresenter(IReflEventTabPresenter *tabPresenter) override;

private:
  std::string logFilterAndSliceValues(std::string const &slicingValues,
                                      std::string const &logFilter) const;
  /// The view we are managing
  IReflEventView *m_view;
  IReflEventTabPresenter *m_tabPresenter;
  SliceType m_sliceType;
  int m_group;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H */
