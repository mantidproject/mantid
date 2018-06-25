#ifndef MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWPRESENTER_H

#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "IReflMainWindowView.h"
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
class IReflMainWindowPresenter : public ReflMainWindowSubscriber {
public:
  virtual std::string runPythonAlgorithm(const std::string &pythonCode) = 0;
//  virtual void setInstrumentName(const std::string &instName) const = 0;
  virtual bool isProcessing() const = 0;
  virtual ~IReflMainWindowPresenter() = default;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWPRESENTER_H */
