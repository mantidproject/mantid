#ifndef MANTID_ISISREFLECTOMETRY_REFLSETTINGSPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_REFLSETTINGSPRESENTER_H

#include "DllConfig.h"
#include "IReflSettingsPresenter.h"
#include "IReflSettingsTabPresenter.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include <initializer_list>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflSettingsView;

/** @class ReflSettingsPresenter

ReflSettingsPresenter is a presenter class for the widget 'Settings' in the
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
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflSettingsPresenter
    : public IReflSettingsPresenter {
public:
  /// Constructor
  ReflSettingsPresenter(IReflSettingsView *view, int id);
  /// Destructor
  ~ReflSettingsPresenter() override;
  void notify(IReflSettingsPresenter::Flag flag) override;
  void setInstrumentName(const std::string &instName) override;

  /// Returns per-angle values passed for 'Transmission run(s)'
  MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angleToFind) const override;
  /// Returns default values passed for 'Transmission run(s)'
  MantidWidgets::DataProcessor::OptionsQMap getDefaultOptions() const;
  /// Whether per-angle transmission runs are specified
  bool hasPerAngleOptions() const override;
  /// Returns global options for 'CreateTransmissionWorkspaceAuto'
  MantidWidgets::DataProcessor::OptionsQMap
  getTransmissionOptions() const override;
  /// Returns global options for 'ReflectometryReductionOneAuto'
  MantidWidgets::DataProcessor::OptionsQMap
  getReductionOptions() const override;
  /// Returns global options for 'Stitch1DMany'
  std::string getStitchOptions() const override;

  void acceptTabPresenter(IReflSettingsTabPresenter *tabPresenter) override;
  void onReductionPaused() override;
  void onReductionResumed() override;
  Mantid::API::IAlgorithm_sptr createReductionAlg() override;

private:
  void createStitchHints();
  void getExpDefaults();
  void getInstDefaults();
  void handleSettingsChanged();
  bool hasReductionTypes(const std::string &summationType) const;
  bool hasIncludePartialBinsOption(const std::string &summationType) const;
  void handleSummationTypeChange();
  static QString asAlgorithmPropertyBool(bool value);
  Mantid::Geometry::Instrument_const_sptr
  createEmptyInstrument(const std::string &instName);

  MantidWidgets::DataProcessor::OptionsQMap transmissionOptionsMap() const;
  void addIfNotEmpty(MantidWidgets::DataProcessor::OptionsQMap &options,
                     const QString &key, const QString &value) const;
  void addIfNotEmpty(MantidWidgets::DataProcessor::OptionsQMap &options,
                     const QString &key, const std::string &value) const;
  void setTransmissionOption(MantidWidgets::DataProcessor::OptionsQMap &options,
                             const QString &key, const QString &value) const;
  void setTransmissionOption(MantidWidgets::DataProcessor::OptionsQMap &options,
                             const QString &key,
                             const std::string &value) const;
  void
  addTransmissionOptions(MantidWidgets::DataProcessor::OptionsQMap &options,
                         std::initializer_list<QString> keys) const;
  QString getProcessingInstructions() const;

  /// The view we are managing
  IReflSettingsView *m_view;
  IReflSettingsTabPresenter *m_tabPresenter;
  /// Name of the current instrument in use
  std::string m_currentInstrumentName;
  int m_group;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_REFLSETTINGSPRESENTER_H */
