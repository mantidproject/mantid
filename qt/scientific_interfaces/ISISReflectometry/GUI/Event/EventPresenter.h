// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H

#include "../../Reduction/Slicing.h"
#include "Common/DllConfig.h"
#include "IEventPresenter.h"
#include "IEventView.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class EventPresenter

EventPresenter is a presenter class for the widget 'Event' in the
ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL EventPresenter
    : public IEventPresenter,
      public EventViewSubscriber {
public:
  explicit EventPresenter(IEventView *view);

  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  void reductionPaused() override;
  void reductionResumed() override;
  void autoreductionPaused() override;
  void autoreductionResumed() override;

  void notifySliceTypeChanged(SliceType newSliceType) override;
  void notifyUniformSliceCountChanged(int sliceCount) override;
  void notifyUniformSecondsChanged(double sliceLengthInSeconds) override;
  void
  notifyCustomSliceValuesChanged(std::string pythonListOfSliceTimes) override;
  void
  notifyLogSliceBreakpointsChanged(std::string logValueBreakpoints) override;
  void notifyLogBlockNameChanged(std::string blockName) override;

  Slicing const &slicing() const override;

private:
  IBatchPresenter *m_mainPresenter;
  Slicing m_slicing;
  void setUniformSlicingByNumberOfSlicesFromView();
  void setUniformSlicingByTimeFromView();
  void setCustomSlicingFromView();
  void setLogValueSlicingFromView();
  void setSlicingFromView();
  void updateWidgetEnabledState() const;
  bool isProcessing() const;
  bool isAutoreducing() const;
  /// The view we are managing
  IEventView *m_view;
  SliceType m_sliceType;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_REFLEVENTPRESENTER_H */
