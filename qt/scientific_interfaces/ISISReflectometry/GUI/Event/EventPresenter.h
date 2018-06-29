#ifndef MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H

#include "../../DllConfig.h"
#include "IEventPresenter.h"
#include "IReflBatchPresenter.h"
#include "IEventView.h"
#include "../../Reduction/Slicing.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class EventPresenter

EventPresenter is a presenter class for the widget 'Event' in the
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
class MANTIDQT_ISISREFLECTOMETRY_DLL EventPresenter
    : public IEventPresenter,
      public EventTabViewSubscriber {
public:
  EventPresenter(IEventView *view);

  void onReductionPaused() override;
  void onReductionResumed() override;

  void notifySliceTypeChanged(SliceType newSliceType) override;
  void notifyUniformSliceCountChanged(int sliceCount) override;
  void notifyUniformSecondsChanged(double sliceLengthInSeconds) override;
  void
  notifyCustomSliceValuesChanged(std::string pythonListOfSliceTimes) override;
  void
  notifyLogSliceBreakpointsChanged(std::string logValueBreakpoints) override;
  void notifyLogBlockNameChanged(std::string blockName) override;

  void acceptMainPresenter(IReflBatchPresenter *mainPresenter) override;

  Slicing const &slicing() const;

private:
  Slicing m_slicing;
  void setUniformSlicingByNumberOfSlicesFromView();
  void setUniformSlicingByTimeFromView();
  void setCustomSlicingFromView();
  void setLogValueSlicingFromView();
  void setSlicingFromView();
  /// The view we are managing
  IEventView *m_view;
  IReflBatchPresenter *m_mainPresenter;
  SliceType m_sliceType;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H */
