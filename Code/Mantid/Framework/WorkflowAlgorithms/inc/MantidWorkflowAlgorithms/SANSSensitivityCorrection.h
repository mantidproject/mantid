#ifndef MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_
#define MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

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
  /// (Empty) Constructor
  SANSSensitivityCorrection() : API::Algorithm() { m_output_message = ""; }
  /// Virtual destructor
  virtual ~SANSSensitivityCorrection() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SANSSensitivityCorrection"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Perform SANS sensitivity correction.";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Workflow\\SANS\\UsesPropertyManager";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  /// Check whether we have a processed file of not
  bool fileCheck(const std::string &filePath);

  std::string m_output_message;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_*/
