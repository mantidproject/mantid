#ifndef MANTID_KERNEL_INVISIBLEPROPERTY_H_
#define MANTID_KERNEL_INVISIBLEPROPERTY_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IPropertySettings.h"

namespace Mantid {
namespace Kernel {

/** This property setting object makes a property invisible in the GUI.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL InvisibleProperty : public IPropertySettings {
public:
  bool isVisible(const IPropertyManager *) const override;
  IPropertySettings *clone() const override;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_INVISIBLEPROPERTY_H_ */
