#ifndef MANTID_API_INDEXPROPERTY_H_
#define MANTID_API_INDEXPROPERTY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/IndexTypeProperty.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Indexing {
class IndexInfo;
}
namespace API {

/** IndexProperty : Implementation of a property type which returns a
  SpectrumIndexSet provided an input vector of integers or a string. The
  constructor accepts a WorkspaceProperty and an IndexTypeProperty which are
  used to validate user input and access the IndexInfo which manages spectrum
  index information within the workspace.

  @author Lamar Moore
  @date 1/7/2017

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
class MANTID_API_DLL IndexProperty : public Kernel::ArrayProperty<int64_t> {
public:
  IndexProperty(const std::string &name,
                const IWorkspaceProperty &workspaceProp,
                const IndexTypeProperty &indexTypeProp,
                Kernel::IValidator_sptr validator =
                    Kernel::IValidator_sptr(new Kernel::NullValidator));

  IndexProperty *clone() const override;

  using Kernel::ArrayProperty<int64_t>::operator=;

  bool isDefault() const override;
  std::string isValid() const override;
  IndexProperty &operator=(const std::string &rhs);
  operator Indexing::SpectrumIndexSet() const;
  Indexing::SpectrumIndexSet getIndices() const;
  Indexing::IndexInfo getFilteredIndexInfo() const;

  static std::string generatePropertyName(const std::string &name = "");

private:
  const Indexing::IndexInfo &getIndexInfoFromWorkspace() const;

  const IWorkspaceProperty &m_workspaceProp;
  const IndexTypeProperty &m_indexTypeProp;
  mutable Indexing::SpectrumIndexSet m_indices;
  mutable bool m_indicesExtracted;
  std::string m_validString;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_INDEXPROPERTY_H_ */
