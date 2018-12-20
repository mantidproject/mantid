#ifndef MANTID_CRYSTAL_CLEARUB_H_
#define MANTID_CRYSTAL_CLEARUB_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace API {
class ExperimentInfo;
}

namespace Crystal {

/** ClearUB : Clear the UB matrix from a workspace by removing the oriented
  lattice.

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ClearUB : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Clears the UB by removing the oriented lattice from the sample.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SetUB", "HasUB"};
  }
  const std::string category() const override;

protected:
  bool doExecute(Mantid::API::Workspace *const ws, bool dryRun);

private:
  bool
  clearSingleExperimentInfo(Mantid::API::ExperimentInfo *const experimentInfo,
                            const bool dryRun) const;
  void init() override;
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_CLEARUB_H_ */
