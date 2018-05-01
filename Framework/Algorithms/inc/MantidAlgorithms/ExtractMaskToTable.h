#ifndef MANTID_ALGORITHMS_EXTRACTMASKTOTABLE_H_
#define MANTID_ALGORITHMS_EXTRACTMASKTOTABLE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IDTypes.h"

namespace Mantid {
namespace Algorithms {

/** ExtractMaskToTable : Extract the mask in a workspace to a table workspace.
  The table workspace must be compatible to algorithm MaskBinsFromTable.

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
class DLLExport ExtractMaskToTable : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ExtractMaskToTable"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The output TableWorkspace should be compatible to "
           "MaskBinsFromTable.";
  }

  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ExtractMask"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Masking"; }

  /// Remove the items appeared in a vector from another
  std::vector<detid_t> subtractVector(std::vector<detid_t> minuend,
                                      std::vector<detid_t> subtrahend);

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Input matrix workspace
  API::MatrixWorkspace_const_sptr m_dataWS;
  /// Input table workspace
  DataObjects::TableWorkspace_sptr m_inputTableWS;

  /// Parse input TableWorkspace to get a list of detectors IDs of which
  /// detector are already masked
  std::vector<detid_t>
  parseMaskTable(DataObjects::TableWorkspace_sptr masktablews);

  /// Parse a string containing list in format (x, xx-yy, x, x, ...) to a vector
  /// of detid_t
  std::vector<detid_t> parseStringToVector(std::string liststr);

  /// Extract mask from a workspace to a list of detectors
  std::vector<detid_t> extractMaskFromMatrixWorkspace();

  /// Extract masked detectors from a MaskWorkspace
  std::vector<detid_t> extractMaskFromMaskWorkspace();

  /// Copy table workspace content from one workspace to another
  void copyTableWorkspaceContent(DataObjects::TableWorkspace_sptr sourceWS,
                                 DataObjects::TableWorkspace_sptr targetWS);

  /// Add a list of spectra (detector IDs) to the output table workspace
  void addToTableWorkspace(DataObjects::TableWorkspace_sptr outws,
                           std::vector<detid_t> maskeddetids, double xmin,
                           double xmax, std::vector<detid_t> prevmaskedids);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EXTRACTMASKTOTABLE_H_ */
