#ifndef MANTID_ISISREFLECTOMETRY_IREFLSETTINGSTABVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLSETTINGSTABVIEW_H

#include "DllConfig.h"
#include "ExperimentOptionDefaults.h"
#include "GetInstrumentParameter.h"
#include "InstrumentOptionDefaults.h"
#include "InstrumentParameters.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include "MantidQtWidgets/Common/Hint.h"
#include <map>
#include <vector>
#include "IReflSettingsTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflEventTabView

IReflSettingsView is the base view class for the Reflectometry settings. It
contains no QT specific functionality as that should be handled by a subclass.

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

class ReflSettingsTabViewSubscriber {
public:
  virtual void notifyExperimentDefaultsRequested() = 0;
  virtual void notifyInstrumentDefaultsRequested() = 0;
  virtual void notifySettingsChanged() = 0;
  virtual void notifySummationTypeChanged() = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IReflSettingsTabView {
public:
  virtual void subscribe(ReflSettingsTabViewSubscriber *notifyee) = 0;
  virtual std::string getStitchOptions() const = 0;
  virtual void
  createStitchHints(const std::vector<MantidWidgets::Hint> &hints) = 0;

  /// Experiment settings
  virtual std::string getAnalysisMode() const = 0;
  virtual std::map<std::string,
                   MantidQt::MantidWidgets::DataProcessor::OptionsQMap>
  getPerAngleOptions() const = 0;
  virtual std::string getStartOverlap() const = 0;
  virtual std::string getEndOverlap() const = 0;
  virtual std::string getPolarisationCorrections() const = 0;
  virtual std::string getCRho() const = 0;
  virtual std::string getCAlpha() const = 0;
  virtual std::string getCAp() const = 0;
  virtual std::string getCPp() const = 0;
  /// Instrument settings
  virtual std::string getIntMonCheck() const = 0;
  virtual std::string getMonitorIntegralMin() const = 0;
  virtual std::string getMonitorIntegralMax() const = 0;
  virtual std::string getMonitorBackgroundMin() const = 0;
  virtual std::string getMonitorBackgroundMax() const = 0;
  virtual std::string getLambdaMin() const = 0;
  virtual std::string getLambdaMax() const = 0;
  virtual std::string getI0MonitorIndex() const = 0;
  virtual std::string getDetectorCorrectionType() const = 0;
  virtual std::string getSummationType() const = 0;
  virtual std::string getReductionType() const = 0;

  /// Check if settings are enabled
  virtual bool experimentSettingsEnabled() const = 0;
  virtual bool instrumentSettingsEnabled() const = 0;
  virtual bool detectorCorrectionEnabled() const = 0;

  /// Set default values for settings
  virtual void setExpDefaults(ExperimentOptionDefaults defaults) = 0;
  virtual void setInstDefaults(InstrumentOptionDefaults defaults) = 0;
  virtual void showOptionLoadErrors(
      std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
      std::vector<MissingInstrumentParameterValue> const &missingValues) = 0;

  /// Sets status of whether polarisation corrections should be enabled/disabled
  virtual void setIsPolCorrEnabled(bool enable) const = 0;
  virtual void setReductionTypeEnabled(bool enable) = 0;
  /// Set polarisation corrections and parameters enabled/disabled
  virtual void setPolarisationOptionsEnabled(bool enable) = 0;
  virtual void setDetectorCorrectionEnabled(bool enable) = 0;
  virtual void disableAll() = 0;
  virtual void enableAll() = 0;
  virtual ~IReflSettingsTabView() = default;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_IREFLSETTINGSVIEW_H */
