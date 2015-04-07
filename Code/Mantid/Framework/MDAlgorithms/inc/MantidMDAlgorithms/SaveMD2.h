#ifndef MANTID_MDALGORITHMS_SAVEMD2_H_
#define MANTID_MDALGORITHMS_SAVEMD2_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"

namespace Mantid {

namespace MDAlgorithms {

/** Save a MDEventWorkspace to a .nxs file.

  @author Janik Zikovsky
  @date 2011-07-11

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  SaveMD2();
  ~SaveMD2();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "SaveMD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Save a MDEventWorkspace or MDHistoWorkspace to a .nxs file.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 2; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  /// Helper method
  template <typename MDE, size_t nd>
  void doSaveEvents(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Save the MDHistoWorkspace.
  void doSaveHisto(Mantid::DataObjects::MDHistoWorkspace_sptr ws);

  /// Save all the affine matricies
  void saveAffineTransformMatricies(::NeXus::File *const file,
                                    API::IMDWorkspace_const_sptr ws);
  /// Save a given affine matrix
  void saveAffineTransformMatrix(::NeXus::File *const file,
                                 API::CoordTransform *transform,
                                 std::string entry_name);
  /// Save a generic matrix
  template <typename T>
  void saveMatrix(::NeXus::File *const file, std::string name,
                  Kernel::Matrix<T> &m, ::NeXus::NXnumtype type,
                  std::string tag = "");
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SAVEMD2_H_ */
