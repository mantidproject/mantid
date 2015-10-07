#ifndef MANTID_MDALGORITHMS_SLICEMD_H_
#define MANTID_MDALGORITHMS_SLICEMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"

namespace Mantid {
namespace MDAlgorithms {

/** Algorithm that can take a slice out of an original MDEventWorkspace
 * while preserving all the events contained wherein.

  @author Janik Zikovsky
  @date 2011-09-27

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
class DLLExport SliceMD : public SlicingAlgorithm {
public:
  //// enum describes situations, which could happen for input Box binning
  // enum BoxState
  //{
  //  boxTooSmall,  // too few events in the box worth considering its vertices
  //  boxOutside,   // box is not small but is completely out of slice
  //  boxWorthConsidering // box belongs to slice and its events worth
  //  considering for slicing
  //};

  SliceMD();
  ~SliceMD();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "SliceMD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Make a MDEventWorkspace containing the events in a slice of an "
           "input MDEventWorkspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  /// Helper method
  template <typename MDE, size_t nd>
  void doExec(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Method to actually do the slice
  template <typename MDE, size_t nd, typename OMDE, size_t ond>
  void slice(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

protected: // for testing
  /*  /// Method to slice box's events if the box itself belongs to the slice
    template<typename MDE, size_t nd, typename OMDE, size_t ond>
    void sliceMDBox(MDBox<MDE, nd> * box, size_t * chunkMin, size_t * chunkMax);

    template<typename MDE, size_t nd, typename OMDE, size_t ond>
    BoxState foundBoxState(MDBox<MDE, nd> * box, size_t * chunkMin, size_t *
    chunkMax);*/
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SLICEMD_H_ */
