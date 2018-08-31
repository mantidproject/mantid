#ifndef MANTID_ISISREFLECTOMETRY_IEXPERIMENTVIEW_H
#define MANTID_ISISREFLECTOMETRY_IEXPERIMENTVIEW_H

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

/** @class IExperimentView

IExperimentView is the base view class for the Reflectometry experiment
settings. It
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

class MANTIDQT_ISISREFLECTOMETRY_DLL ExperimentViewSubscriber {
public:
  virtual void notifyPerAngleDefaultsChanged(int column, int row) = 0;
  virtual void notifySettingsChanged() = 0;
  virtual void notifySummationTypeChanged() = 0;
  virtual void notifyNewPerAngleDefaultsRequested() = 0;
  virtual void notifyRemovePerAngleDefaultsRequested(int index) = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IExperimentView {
public:
  virtual void subscribe(ExperimentViewSubscriber *notifyee) = 0;
  virtual void
  createStitchHints(const std::vector<MantidWidgets::Hint> &hints) = 0;

  virtual std::string getAnalysisMode() const = 0;
  virtual void setAnalysisMode(std::string const &analysisMode) = 0;

  virtual std::string getSummationType() const = 0;
  virtual void setSummationType(std::string const &summationType) = 0;

  virtual std::string getReductionType() const = 0;
  virtual void setReductionType(std::string const &reductionType) = 0;
  virtual void enableReductionType() = 0;
  virtual void disableReductionType() = 0;

  virtual std::vector<std::array<std::string, 8>>
  getPerAngleOptions() const = 0;
  virtual void showPerAngleOptionsAsInvalid(int row, int column) = 0;
  virtual void showPerAngleOptionsAsValid(int row) = 0;
  virtual void showAllPerAngleOptionsAsValid() = 0;
  virtual void showStitchParametersValid() = 0;
  virtual void showStitchParametersInvalid() = 0;

  virtual void enablePolarisationCorrections() = 0;
  virtual void disablePolarisationCorrections() = 0;

  virtual double getTransmissionStartOverlap() const = 0;
  virtual void setTransmissionStartOverlap(double start) = 0;
  virtual double getTransmissionEndOverlap() const = 0;
  virtual void setTransmissionEndOverlap(double end) = 0;

  virtual std::string getPolarisationCorrectionType() const = 0;
  virtual void setPolarisationCorrectionType(std::string const &type) = 0;
  virtual double getCRho() const = 0;
  virtual void setCRho(double cRho) = 0;
  virtual double getCAlpha() const = 0;
  virtual void setCAlpha(double cAlpha) = 0;
  virtual double getCAp() const = 0;
  virtual void setCAp(double cAp) = 0;
  virtual double getCPp() const = 0;
  virtual void setCPp(double cPp) = 0;

  virtual std::string getStitchOptions() const = 0;
  virtual void setStitchOptions(std::string const &stitchOptions) = 0;

  virtual void showOptionLoadErrors(
      std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
      std::vector<MissingInstrumentParameterValue> const &missingValues) = 0;

  virtual void disableAll() = 0;
  virtual void enableAll() = 0;

  virtual void addPerThetaDefaultsRow() = 0;
  virtual void removePerThetaDefaultsRow(int rowIndex) = 0;

  virtual void showPerAngleThetasNonUnique(double tolerance) = 0;

  virtual ~IExperimentView() = default;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_IEXPERIMENTVIEW_H */
