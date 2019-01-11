// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
