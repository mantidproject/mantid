#ifndef MANTID_API_INDEXTYPEPROPERTY_H_
#define MANTID_API_INDEXTYPEPROPERTY_H_

#include "MantidAPI/DllConfig.h"
#include <MantidKernel/PropertyWithValue.h>

namespace Mantid {
namespace API {

enum IndexType { SpectrumNumber = 1, WorkspaceIndex = 2 };

/** IndexTypeProperty : TODO : Add support for
DetectorID->SpectrumNumber/WorkspaceIndex
        @author Lamar Moore
        @Date 05-05-2017
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
class MANTID_API_DLL IndexTypeProperty
    : public Kernel::PropertyWithValue<std::string> {
public:
  IndexTypeProperty(const int indexType);

  IndexType selectedType() const;

  int allowedTypes() const;

  std::vector<std::string> allowedValues() const override;

  bool isMultipleSelectionAllowed() override;

  using PropertyWithValue<std::string>::operator=;

  std::string &operator=(API::IndexType type);

private:
  std::vector<std::string> m_allowedValues;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_INDEXTYPEPROPERTY_H_ */