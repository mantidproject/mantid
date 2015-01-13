#ifndef MANTID_CRYSTAL_FIND_UB_USING_INDEXED_PEAKS_H_
#define MANTID_CRYSTAL_FIND_UB_USING_INDEXED_PEAKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {
/** FindUBUsingIndexedPeaks : Algorithm to calculate a UB matrix,
    given a list of peaks that have already been indexed by some means.

    @author Dennis Mikkelson(adapted from Andrei Savici's CalculateUMatrix)
    @date   2011-08-17

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory &
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
class DLLExport FindUBUsingIndexedPeaks : public API::Algorithm {
public:
  FindUBUsingIndexedPeaks();
  ~FindUBUsingIndexedPeaks();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "FindUBUsingIndexedPeaks"; };

  /// Algorithm's version for identification
  virtual int version() const { return 1; };

  /// Algorithm's category for identification
  virtual const std::string category() const { return "Crystal"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculate the UB matrix from a peaks workspace, containing indexed "
           "peaks.";
  }

private:
  /// Initialise the properties
  void init();

  /// Run the algorithm
  void exec();
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_FIND_UB_USING_INDEXED_PEAKS */
