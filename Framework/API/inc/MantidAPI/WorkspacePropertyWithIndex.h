#ifndef MANTID_API_WORKSPACEPROPERTYWITHINDEX_H_
#define MANTID_API_WORKSPACEPROPERTYWITHINDEX_H_

#include "MantidAPI/IWorkspacePropertyWithIndex.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <MantidAPI/IndexTypeProperty.h>
#include <MantidIndexing/GlobalSpectrumIndex.h>
#include <MantidIndexing/SpectrumIndexSet.h>
#include <MantidIndexing/SpectrumNumber.h>
#include <MantidKernel/ArrayProperty.h>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <tuple>
#include <vector>

namespace Mantid {
namespace API {
class MatrixWorkspace;

/** WorkspacePropertyWithIndex : Workspace property which allows users to
  specify an input index type. The property then simultaneously returns the
  workspace and the indices.
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
template <typename TYPE = MatrixWorkspace>
class WorkspacePropertyWithIndex : public WorkspaceProperty<TYPE>,
                                   public IWorkspacePropertyWithIndex {
  static_assert(std::is_convertible<TYPE *, MatrixWorkspace *>::value,
                "Workspace type must be convertible to API::MatrixWorkspace.");
public:
  explicit WorkspacePropertyWithIndex(
      const std::string &name = "InputWorkspaceWithIndex",
      const int indexTypes = IndexType::WorkspaceIndex,
      const std::string &wsName = "",
      Kernel::IValidator_sptr validator =
          Kernel::IValidator_sptr(new Kernel::NullValidator));

  explicit WorkspacePropertyWithIndex(
      const std::string &name, const int indexTypes, const std::string &wsName,
      API::PropertyMode::Type optional,
      Kernel::IValidator_sptr validator =
          Kernel::IValidator_sptr(new Kernel::NullValidator));

  explicit WorkspacePropertyWithIndex(
      const std::string &name, const int indexTypes, const std::string &wsName,
      API::PropertyMode::Type optional, API::LockMode::Type locking,
      Kernel::IValidator_sptr validator =
          Kernel::IValidator_sptr(new Kernel::NullValidator));

  WorkspacePropertyWithIndex(const WorkspacePropertyWithIndex<TYPE> &right);

  using WorkspaceProperty<TYPE>::operator=;

  std::string isValid() const override;

  bool operator==(const WorkspacePropertyWithIndex<TYPE> &rhs);
  WorkspacePropertyWithIndex &
  operator=(const std::tuple<boost::shared_ptr<TYPE>, API::IndexType,
                             std::vector<int>> &rhs);
  WorkspacePropertyWithIndex &operator=(
      const std::tuple<boost::shared_ptr<TYPE>, API::IndexType, std::string>
          &rhs);
  WorkspacePropertyWithIndex &
  operator=(const WorkspacePropertyWithIndex<TYPE> &rhs);
  WorkspacePropertyWithIndex &
  operator+=(WorkspacePropertyWithIndex<TYPE> const *rhs);

  operator const std::tuple<boost::shared_ptr<TYPE>,
                            Indexing::SpectrumIndexSet>() const;

  operator const std::tuple<boost::shared_ptr<const TYPE>,
                            Indexing::SpectrumIndexSet>() const;

  WorkspacePropertyWithIndex *clone() const override;

  Kernel::ArrayProperty<int> &mutableIndexListProperty() {
    return *m_indexListProp.get();
  }

  const Kernel::ArrayProperty<int> &indexListProperty() const {
    return *m_indexListProp;
  }

  IndexTypeProperty &mutableIndexTypeProperty() {
    return *m_indexTypeProp.get();
  }
  const IndexTypeProperty &indexTypeProperty() const {
    return *m_indexTypeProp;
  }

private:
  std::unique_ptr<Kernel::ArrayProperty<int>> m_indexListProp;
  std::unique_ptr<IndexTypeProperty> m_indexTypeProp;

  const Indexing::SpectrumIndexSet getIndices() const;
};
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACEPROPERTYWITHINDEX_H_ */