#ifndef MANTID_CUSTOMINTERFACES_IREFLSETTINGSPRESENTER_H
#define MANTID_CUSTOMINTERFACES_IREFLSETTINGSPRESENTER_H

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
  /// Pre-processing
  virtual std::string getTransmissionRuns(bool loadRuns) const = 0;
  virtual std::string getTransmissionOptions() const = 0;
  /// Processing
  virtual std::string getReductionOptions() const = 0;
  /// Post-processing
  virtual std::string getStitchOptions() const = 0;

  enum Flag { ExpDefaultsFlag, InstDefaultsFlag };

  /// Tell the presenter something happened
  virtual void notify(IReflSettingsPresenter::Flag flag) = 0;
  /// Set current instrument name
  virtual void setInstrumentName(const std::string &instName) = 0;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_IREFLSETTINGSPRESENTER_H */
