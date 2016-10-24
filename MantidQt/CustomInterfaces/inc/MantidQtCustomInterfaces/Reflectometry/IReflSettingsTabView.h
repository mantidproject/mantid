#ifndef MANTID_CUSTOMINTERFACES_IREFLSETTINGSTABVIEW_H
#define MANTID_CUSTOMINTERFACES_IREFLSETTINGSTABVIEW_H

#include "MantidQtCustomInterfaces/DllConfig.h"
#include <map>

namespace MantidQt {

namespace CustomInterfaces {

class IReflSettingsTabPresenter;

/** @class IReflSettingsTabView

IReflSettingsTabView is the base view class for the Reflectometry Interface. It
contains
no QT specific functionality as that should be handled by a subclass.

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

class DLLExport IReflSettingsTabView {
public:
  /// Constructor
  IReflSettingsTabView(){};
  /// Destructor
  virtual ~IReflSettingsTabView(){};
  /// Returns the presenter managing this view
  virtual IReflSettingsTabPresenter *getPresenter() const = 0;

  /// Pre-processing
  virtual std::string getPlusOptions() const = 0;
  virtual std::string getTransmissionOptions() const = 0;
  virtual void
  createPlusHints(const std::map<std::string, std::string> &hints) = 0;
  virtual void
  createTransmissionHints(const std::map<std::string, std::string> &hints) = 0;
  /// Processing
  virtual std::string getReductionOptions() const = 0;
  virtual void
  createReductionHints(const std::map<std::string, std::string> &hints) = 0;
  /// Post-processing
  virtual std::string getStitchOptions() const = 0;
  virtual void
  createStitchHints(const std::map<std::string, std::string> &hints) = 0;

  /// Experiment settings
  virtual std::string getAnalysisMode() const = 0;
  virtual std::string getCRho() const = 0;
  virtual std::string getCAlpha() const = 0;
  virtual std::string getCAp() const = 0;
  virtual std::string getCPp() const = 0;
  virtual std::string getDirectBeam() const = 0;
  virtual std::string getPolarisationCorrections() const = 0;
  /// Instrument settings
  virtual std::string getMonitorIntegralMin() const = 0;
};
}
}
#endif /* MANTID_CUSTOMINTERFACES_IREFLRUNSTABVIEW_H */
