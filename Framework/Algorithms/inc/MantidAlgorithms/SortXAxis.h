#ifndef MANTID_ALGORITHMS_SORTXAXIS_H_
#define MANTID_ALGORITHMS_SORTXAXIS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace Algorithms {

/** SortXAxis
 
  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  @author Samuel Jones
*/

class DLLExport SortXAxis : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private: 
  void init() override;
  void exec() override;
  void createIndexes(std::vector<std::size_t>, const size_t);
  void orderIndexesDecending(std::vector<std::size_t>,
                             Mantid::API::MatrixWorkspace_const_sptr,
                             unsigned int);
  void orderIndexesAscending(std::vector<std::size_t>,
                             Mantid::API::MatrixWorkspace_const_sptr,
                             unsigned int);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SORTXAXIS_H_ */