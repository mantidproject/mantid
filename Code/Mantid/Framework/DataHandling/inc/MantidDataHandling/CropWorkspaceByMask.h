#ifndef MANTID_DATAHANDLING_CROPWORKSPACEBYMASK_H_
#define MANTID_DATAHANDLING_CROPWORKSPACEBYMASK_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/** CropWorkspaceByMask crops a MatrixWorkspace by a mask. It removes all masked
  spectra.

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
class DLLExport CropWorkspaceByMask : public API::Algorithm {
public:
  CropWorkspaceByMask();
  virtual ~CropWorkspaceByMask();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

private:
  void init();
  void exec();
  /// Fill in a vector with spectra indices to be extracted.
  void makeIndexList(std::vector<size_t>& indices, const API::MatrixWorkspace* maskedWorkspace);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_CROPWORKSPACEBYMASK_H_ */