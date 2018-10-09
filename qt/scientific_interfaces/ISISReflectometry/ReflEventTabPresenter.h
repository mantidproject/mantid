// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
