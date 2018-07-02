#ifndef MANTID_ALGORITHMS_MASKDETECTORSIF_H_
#define MANTID_ALGORITHMS_MASKDETECTORSIF_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/System.h"
#include <boost/function.hpp>

// To be compatible with VSC Express edition that does not have tr1

#include <unordered_map>

namespace Mantid {
namespace Algorithms {
/**
 *
This algorithm is used to select/deselect detectors in a *.cal file.


 @author Laurent Chapon, Pascal Manuel ISIS Facility, Rutherford Appleton
Laboratory
 @date 06/07/2009

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MaskDetectorsIf : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MaskDetectorsIf"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Adjusts the selected field for a CalFile depending on the values "
           "in the input workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\Masking;Transforms\\Masking";
  }

private:
  /// Returns an allowed values statement to insert into decumentation
  std::string allowedValuesStatement(const std::vector<std::string> &vals);
  // Typedef for det to value map
  using udet2valuem = std::unordered_map<detid_t, bool>;
  /// A map of detector numbers to mask boolean
  udet2valuem umap;
  /// Get the properties
  void retrieveProperties();
  /// Create a new cal file
  void createNewCalFile(const std::string &oldfile, const std::string &newfile);
  /// The input workspace
  API::MatrixWorkspace_const_sptr inputW;
  /// The Value parameter
  double value = 0.0;
  /// A comparator function
  boost::function<bool(double, double)> compar_f;
  /// Whether select is on or off
  bool select_on = false;
  /// Overidden init
  void init() override;
  /// Overidden exec
  void exec() override;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MASKDETECTORSIF_H_*/
