#ifndef MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_
#define MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/PropertyManager.h"

#include <Poco/Path.h>

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Sensitivity correction for SANS

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
class DLLExport SANSSensitivityCorrection : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "SANSSensitivityCorrection";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Perform SANS sensitivity correction.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Workflow\\SANS\\UsesPropertyManager";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Check whether we have a processed file of not
  bool fileCheck(const std::string &filePath);
  /// Process the flood or it's background
  void process(API::MatrixWorkspace_sptr rawFloodWS,
               const std::string reductionManagerName,
               boost::shared_ptr<Kernel::PropertyManager> reductionManager,
               const std::string propertyPrefix = "");
  /// Load the flood or it's background
  API::MatrixWorkspace_sptr
  load(boost::shared_ptr<Kernel::PropertyManager> reductionManager,
       const std::string fileName, Poco::Path path);
  std::string m_output_message;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_*/
