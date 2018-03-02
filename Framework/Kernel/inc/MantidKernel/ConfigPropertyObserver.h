#ifndef MANTID_KERNEL_CONFIGPROPERTYOBSERVER_H_
#define MANTID_KERNEL_CONFIGPROPERTYOBSERVER_H_
#include "ConfigService.h"
#include "ConfigObserver.h"
#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {
/** The ConfigObserver is used to observe changes to the value of a single
   configuration property based on notifications sent from the ConfigService.

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class MANTID_KERNEL_DLL ConfigPropertyObserver : ConfigObserver {
public:
  ConfigPropertyObserver(std::string propertyName);
  virtual ~ConfigPropertyObserver() = default;

protected:
  void onValueChanged(const std::string &name, const std::string &newValue,
                      const std::string &prevValue) override;

  virtual void onPropertyValueChanged(const std::string &newValue,
                                      const std::string &prevValue) = 0;

private:
  std::string m_propertyName;
};
}
}
#endif /*MANTID_KERNEL_CONFIGPROPERTYOBSERVER_H_*/
