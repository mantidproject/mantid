#ifndef MANTID_ALGORITHMS_PERFORMINDEXOPERATIONS_H_
#define MANTID_ALGORITHMS_PERFORMINDEXOPERATIONS_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** PerformIndexOperations : Crop and sum a workspace according to the parsed
  workspace index operations provided.

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
class DLLExport PerformIndexOperations : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Process the workspace according to the Index operations provided.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractSpectra"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_PERFORMINDEXOPERATIONS_H_ */
