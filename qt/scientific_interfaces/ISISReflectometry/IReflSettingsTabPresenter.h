#ifndef MANTID_ISISREFLECTOMETRY_IREFLSETTINGSTABPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLSETTINGSTABPRESENTER_H

#include "IReflBatchPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {


/** @class IReflSettingsTabPresenter

IReflSettingsTabPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Settings' tab presenter

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
class IReflSettingsTabPresenter {
public:
  virtual ~IReflSettingsTabPresenter(){};
  /// Transmission runs for a particular run angle
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angle) const = 0;
  /// Whether per-angle transmission runs are specified
  virtual bool hasPerAngleOptions() const = 0;
  /// Pre-processing
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getTransmissionOptions() const = 0;
  /// Processing
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getReductionOptions() const = 0;
  /// Post-processing
  virtual std::string getStitchOptions() const = 0;
  /// Set current instrument name
  virtual void setInstrumentName(const std::string &instName) = 0;
  virtual void acceptMainPresenter(IReflBatchPresenter *mainPresenter) = 0;
  virtual void settingsChanged() = 0;
  virtual void onReductionPaused() = 0;
  virtual void onReductionResumed() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLSETTINGSTABPRESENTER_H */
