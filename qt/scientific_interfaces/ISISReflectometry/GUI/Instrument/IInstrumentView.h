#ifndef MANTID_ISISREFLECTOMETRY_IINSTRUMENTVIEW_H
#define MANTID_ISISREFLECTOMETRY_IINSTRUMENTVIEW_H

#include "DllConfig.h"
#include "InstrumentParameters.h"
#include "MantidAPI/Algorithm.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class IInstrumentView

IInstrumentView is the base view class for the Reflectometry instrument
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

class MANTIDQT_ISISREFLECTOMETRY_DLL InstrumentViewSubscriber {
public:
  virtual void notifySettingsChanged() = 0;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IInstrumentView {
public:
  virtual ~IInstrumentView() = default;

  virtual void subscribe(InstrumentViewSubscriber *notifyee) = 0;

  virtual int getMonitorIndex() const = 0;
  virtual bool getIntegrateMonitors() const = 0;

  virtual double getLambdaMin() const = 0;
  virtual double getLambdaMax() const = 0;
  virtual void showLambdaRangeInvalid() = 0;
  virtual void showLambdaRangeValid() = 0;

  virtual double getMonitorBackgroundMin() const = 0;
  virtual double getMonitorBackgroundMax() const = 0;
  virtual void showMonitorBackgroundRangeInvalid() = 0;
  virtual void showMonitorBackgroundRangeValid() = 0;

  virtual double getMonitorIntegralMin() const = 0;
  virtual double getMonitorIntegralMax() const = 0;
  virtual void showMonitorIntegralRangeInvalid() = 0;
  virtual void showMonitorIntegralRangeValid() = 0;

  virtual bool getCorrectDetectors() const = 0;
  virtual std::string getDetectorCorrectionType() const = 0;

  virtual void disableAll() = 0;
  virtual void enableAll() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IINSTRUMENTVIEW_H */
