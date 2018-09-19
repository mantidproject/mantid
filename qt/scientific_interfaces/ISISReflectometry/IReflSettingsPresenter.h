#ifndef MANTID_ISISREFLECTOMETRY_IREFLSETTINGSPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLSETTINGSPRESENTER_H

#include "IReflSettingsTabPresenter.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IReflMainWindowPresenter;

/** @class IReflSettingsPresenter

IReflSettingsPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Settings' presenter

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
class IReflSettingsPresenter {
public:
  virtual ~IReflSettingsPresenter(){};
  /// Transmission runs for a particular angle
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getOptionsForAngle(const double angle) const = 0;
  /// Whether per-angle transmission runs are set
  virtual bool hasPerAngleOptions() const = 0;
  /// Pre-processing
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getTransmissionOptions() const = 0;
  /// Processing
  virtual Mantid::API::IAlgorithm_sptr createReductionAlg() = 0;
  virtual MantidWidgets::DataProcessor::OptionsQMap
  getReductionOptions() const = 0;
  /// Post-processing
  virtual std::string getStitchOptions() const = 0;
  virtual void acceptTabPresenter(IReflSettingsTabPresenter *tabPresenter) = 0;

  enum Flag {
    ExpDefaultsFlag,
    InstDefaultsFlag,
    SettingsChangedFlag,
    SummationTypeChanged
  };

  /// Tell the presenter something happened
  virtual void notify(IReflSettingsPresenter::Flag flag) = 0;
  /// Set current instrument name
  virtual void setInstrumentName(const std::string &instName) = 0;

  virtual void onReductionPaused() = 0;
  virtual void onReductionResumed() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLSETTINGSPRESENTER_H */
