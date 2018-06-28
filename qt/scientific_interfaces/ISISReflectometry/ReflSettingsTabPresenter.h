#ifndef MANTID_ISISREFLECTOMETRY_REFLSETTINGSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLSETTINGSTABPRESENTER_H

#include "DllConfig.h"
#include "IReflSettingsTabPresenter.h"
#include "IReflSettingsTabView.h"
#include "IReflBatchPresenter.h"
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/** @class ReflSettingsTabPresenter

ReflSettingsTabPresenter is a presenter class for the tab 'Settings' in the
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
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflSettingsTabPresenter
    : public IReflSettingsTabPresenter,
      public ReflSettingsTabViewSubscriber {
public:
  /// Constructor
  ReflSettingsTabPresenter(IReflSettingsTabView *view);
  /// Set the instrument name
  void setInstrumentName(const std::string &instName) override;
  void acceptMainPresenter(IReflBatchPresenter *mainPresenter) override;
  void settingsChanged() override;
  void onReductionPaused() override;
  void onReductionResumed() override;
  /// Returns values passed for 'Transmission run(s)'
  MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angle) const override;
  /// Whether per-angle tranmsission runs are set
  bool hasPerAngleOptions() const override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  MantidWidgets::DataProcessor::OptionsQMap
  getTransmissionOptions() const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  MantidWidgets::DataProcessor::OptionsQMap
  getReductionOptions() const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions() const override;
  MantidWidgets::DataProcessor::OptionsQMap getDefaultOptions() const;

  void setTransmissionOption(MantidWidgets::DataProcessor::OptionsQMap &options,
                             const QString &key, const QString &value) const;
  void setTransmissionOption(MantidWidgets::DataProcessor::OptionsQMap &options,
                             const QString &key,
                             const std::string &value) const;

  void notifyExperimentDefaultsRequested() override;
  void notifyInstrumentDefaultsRequested() override;
  void notifySettingsChanged() override;
  void notifySummationTypeChanged() override;
  QString getProcessingInstructions() const;
  void addIfNotEmpty(MantidWidgets::DataProcessor::OptionsQMap &options,
                     const QString &key, const QString &value) const;
  void addIfNotEmpty(MantidWidgets::DataProcessor::OptionsQMap &options,
                     const QString &key, const std::string &value) const;

private:
  Mantid::Geometry::Instrument_const_sptr
  createEmptyInstrument(const std::string &instName);
  Mantid::API::IAlgorithm_sptr createReductionAlg();
  MantidWidgets::DataProcessor::OptionsQMap transmissionOptionsMap() const;
  void
  addTransmissionOptions(MantidWidgets::DataProcessor::OptionsQMap &options,
                         std::initializer_list<QString> keys) const;
  bool hasReductionTypes(const std::string &reductionType) const;
  static QString asAlgorithmPropertyBool(bool value);
  std::string m_currentInstrumentName;

  IReflSettingsTabView *m_view;
  IReflBatchPresenter *m_mainPresenter;

};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_REFLSETTINGSTABPRESENTER_H */
