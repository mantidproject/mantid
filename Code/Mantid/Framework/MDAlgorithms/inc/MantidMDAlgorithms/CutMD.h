#ifndef MANTID_MDALGORITHMS_CUTMD_H_
#define MANTID_MDALGORITHMS_CUTMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid {
namespace MDAlgorithms {

/** CutMD : Slices multidimensional workspaces.

  @date 2015-03-20

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
class DLLExport CutMD : public API::DataProcessorAlgorithm {
public:
  CutMD();
  virtual ~CutMD();

  virtual const std::string name() const { return "CutMD"; }
  virtual int version() const { return 1; }
  virtual const std::string summary() const {
    return "Slices multidimensional workspaces using input projection "
           "information and binning limits.";
  }
  virtual const std::string category() const { return "MDAlgorithms"; }

  virtual void init();
  virtual void exec();

private:
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CUTMD_H_ */
