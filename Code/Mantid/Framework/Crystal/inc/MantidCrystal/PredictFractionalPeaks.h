#ifndef MANTID_CRYSTAL_PREDICTFRACTIONALPEAKS_H_
#define MANTID_CRYSTAL_PREDICTFRACTIONALPEAKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {
/** CreatFractionalPeaks : Algorithm to create a PeaksWorkspace with peaks
   corresponding
    to fractional h,k,and l values.

    @author Ruth Mikkelson
    @date   2012-12-05

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory &
                     NScD Oak Ridge National Laboratory

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

    File change history is stored at:
    <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport PredictFractionalPeaks : public API::Algorithm {
public:
  PredictFractionalPeaks();
  virtual ~PredictFractionalPeaks();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "PredictFractionalPeaks"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "The offsets can be from hkl values in a range of hkl values or "
           "from peaks in the input PeaksWorkspace";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };

  /// Algorithm's category for identification
  virtual const std::string category() const { return "Crystal"; }

private:
  /// Initialise the properties
  void init();

  /// Run the algorithm
  void exec();
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PREDICTFRACTIONALPEAKS */
