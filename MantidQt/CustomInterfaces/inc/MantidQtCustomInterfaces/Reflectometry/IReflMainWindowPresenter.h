#ifndef MANTID_CUSTOMINTERFACES_IREFLMAINWINDOWPRESENTER_H
#define MANTID_CUSTOMINTERFACES_IREFLMAINWINDOWPRESENTER_H

#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflMainWindowPresenter

IReflMainWindowPresenter is the interface defining the functions that the main
window presenter needs to implement. This interface is used by tab presenters to
request information from other tabs.

Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class IReflMainWindowPresenter {
public:
  /// Destructor
  virtual ~IReflMainWindowPresenter(){};
  /// Pre-processing
  virtual std::string getTransmissionOptions(int group) const = 0;
  /// Processing
  virtual std::string getReductionOptions(int group) const = 0;
  /// Post-processing
  virtual std::string getStitchOptions(int group) const = 0;
  /// Time-slicing
  virtual std::string getTimeSlicingOptions(int group) const = 0;
  /// Dialog to print error message
  virtual void giveUserCritical(const std::string &prompt,
                                const std::string &title) = 0;
  /// Dialog to print information
  virtual void giveUserInfo(const std::string &prompt,
                            const std::string &title) = 0;
  /// Run a python algorithm
  virtual std::string runPythonAlgorithm(const std::string &pythonCode) = 0;
  /// Set the instrument name
  virtual void setInstrumentName(const std::string &instName) const = 0;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_IREFLMAINWINDOWPRESENTER_H */
