#ifndef MANTID_MDALGORITHMS_SAVEMD2_H_
#define MANTID_MDALGORITHMS_SAVEMD2_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace MDAlgorithms {

/** Save a MDEventWorkspace to a .nxs file.

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
class DLLExport SaveMD2 : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a MDEventWorkspace or MDHistoWorkspace to a .nxs file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 2; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadMD", "SaveZODS"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\DataHandling";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// Save the MDHistoWorkspace.
  void doSaveHisto(Mantid::DataObjects::MDHistoWorkspace_sptr ws);

  /// Save a generic matrix
  template <typename T>
  void saveMatrix(::NeXus::File *const file, std::string name,
                  Kernel::Matrix<T> &m, ::NeXus::NXnumtype type,
                  std::string tag = "");
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SAVEMD2_H_ */
