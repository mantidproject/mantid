#ifndef MANTID_ALGORITHMS_ADDNOTE_H_
#define MANTID_ALGORITHMS_ADDNOTE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/Run.h"

namespace Mantid {
namespace Algorithms {

/**
  An Algorithm that adds a timestamped note to a workspace.

  @author Elliot Oram, ISIS, RAL
  @date 17/07/2015

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport AddNote : public API::Algorithm,
                          public API::DeprecatedAlgorithm {
public:
  AddNote();

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"Comment"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  /// Remove an existing log of the given name
  void removeExisting(API::MatrixWorkspace_sptr &, const std::string &);
  /// Create or update the named log entry
  void createOrUpdate(API::Run &, const std::string &);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ADDNOTE_H_ */