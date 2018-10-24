// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLSETTINGSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLSETTINGSTABPRESENTER_H

#include "DllConfig.h"
#include "IReflSettingsTabPresenter.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflMainWindowPresenter;
class IReflSettingsPresenter;

/** @class ReflSettingsTabPresenter

ReflSettingsTabPresenter is a presenter class for the tab 'Settings' in the
ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflSettingsTabPresenter
    : public IReflSettingsTabPresenter {
public:
  /// Constructor
  ReflSettingsTabPresenter(std::vector<IReflSettingsPresenter *> presenters);
  /// Destructor
  ~ReflSettingsTabPresenter() override;
  /// Set the instrument name
  void setInstrumentName(const std::string &instName) override;
  void acceptMainPresenter(IReflMainWindowPresenter *mainPresenter) override;
  void settingsChanged(int group) override;
  void onReductionPaused(int group) override;
  void onReductionResumed(int group) override;
  void
  passSelfToChildren(std::vector<IReflSettingsPresenter *> const &children);

  /// Returns values passed for 'Transmission run(s)'
  MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(int group, const double angle) const override;
  /// Whether per-angle tranmsission runs are set
  bool hasPerAngleOptions(int group) const override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  MantidWidgets::DataProcessor::OptionsQMap
  getTransmissionOptions(int group) const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  MantidWidgets::DataProcessor::OptionsQMap
  getReductionOptions(int group) const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions(int group) const override;

private:
  /// The presenters for each group as a vector
  std::vector<IReflSettingsPresenter *> m_settingsPresenters;
  IReflMainWindowPresenter *m_mainPresenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_REFLSETTINGSTABPRESENTER_H */
